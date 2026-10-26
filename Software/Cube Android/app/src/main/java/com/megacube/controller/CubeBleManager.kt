package com.megacube.controller

import android.annotation.SuppressLint
import android.bluetooth.BluetoothAdapter
import android.bluetooth.BluetoothDevice
import android.bluetooth.BluetoothGatt
import android.bluetooth.BluetoothGattCallback
import android.bluetooth.BluetoothGattCharacteristic
import android.bluetooth.BluetoothGattDescriptor
import android.bluetooth.BluetoothGattService
import android.bluetooth.BluetoothManager
import android.bluetooth.BluetoothStatusCodes
import android.bluetooth.le.BluetoothLeScanner
import android.bluetooth.le.ScanCallback
import android.bluetooth.le.ScanFilter
import android.bluetooth.le.ScanResult
import android.bluetooth.le.ScanSettings
import android.content.Context
import android.media.AudioDeviceCallback
import android.media.AudioDeviceInfo
import android.media.AudioManager
import android.os.Build
import android.os.Handler
import android.os.Looper
import android.os.ParcelUuid
import android.util.Log
import java.nio.charset.StandardCharsets
import java.util.UUID
import java.util.ArrayDeque

data class CubeDevice(val name: String, val address: String, val rssi: Int)

enum class ConnectionState { DISCONNECTED, SCANNING, CONNECTING, CONNECTED }

@SuppressLint("MissingPermission")
class CubeBleManager(
    context: Context,
    private val onDevicesChanged: (List<CubeDevice>) -> Unit,
    private val onConnectionChanged: (ConnectionState) -> Unit,
    private val onStatus: (String) -> Unit,
    private val onEvent: (String) -> Unit,
) {
    companion object {
        val SERVICE_UUID: UUID = UUID.fromString("9f6d1000-7d3a-4c2d-9b7d-5b1f52c4a001")
        val COMMAND_UUID: UUID = UUID.fromString("9f6d1001-7d3a-4c2d-9b7d-5b1f52c4a001")
        val STATUS_UUID: UUID = UUID.fromString("9f6d1002-7d3a-4c2d-9b7d-5b1f52c4a001")
        val CCCD_UUID: UUID = UUID.fromString("00002902-0000-1000-8000-00805f9b34fb")
    }

    private val appContext = context.applicationContext
    private val bluetoothManager =
        appContext.getSystemService(BluetoothManager::class.java)
    private val adapter: BluetoothAdapter?
        get() = bluetoothManager?.adapter
    private val scanner: BluetoothLeScanner?
        get() = adapter?.bluetoothLeScanner
    private val devices = linkedMapOf<String, CubeDevice>()
    private val audioManager = appContext.getSystemService(AudioManager::class.java)
    private val mainHandler = Handler(Looper.getMainLooper())

    private val audioDeviceCallback = object : AudioDeviceCallback() {
        override fun onAudioDevicesAdded(addedDevices: Array<out AudioDeviceInfo>) {
            if (state == ConnectionState.CONNECTED) configureAudioLatency()
        }

        override fun onAudioDevicesRemoved(removedDevices: Array<out AudioDeviceInfo>) {
            if (state == ConnectionState.CONNECTED) configureAudioLatency()
        }
    }

    private var gatt: BluetoothGatt? = null
    private var commandCharacteristic: BluetoothGattCharacteristic? = null
    private var state = ConnectionState.DISCONNECTED
    private val commands = ArrayDeque<ByteArray>()
    private var latestMusic: ByteArray? = null
    private var writeInProgress = false
    private var writingMusic = false
    private var musicSelected = false
    private var musicSequence = 0
    private var writeStartRetries = 0
    private var fastConnection = false
    private val catalogPending = ArrayDeque<AnimationChoice>()
    private var catalogAwaiting: Int? = null
    private var catalogRequested = false
    private val catalogTimeout = Runnable {
        val syncing = catalogAwaiting != null
        catalogRequested = false
        catalogAwaiting = null
        catalogPending.clear()
        if (syncing) onStatus("Animation catalog sync timed out; reconnect to retry")
    }
    private val openingOnConnect = Runnable { selectAnimation(31) }
    private val writeTimeout = Runnable {
        if (writeInProgress) {
            onStatus("BLE write timed out; reconnect the cube")
            gatt?.disconnect()
        }
    }

    init {
        audioManager.registerAudioDeviceCallback(audioDeviceCallback, mainHandler)
    }

    private fun handleNotification(value: ByteArray) {
        val message = value.toString(StandardCharsets.UTF_8)
        if (message.startsWith("CATALOG 1 ") && catalogRequested) {
            val defaultCount = message.substringAfterLast(' ').toIntOrNull()
            if (defaultCount != null && defaultCount in 0..256) {
                mainHandler.removeCallbacks(catalogTimeout)
                catalogRequested = false
                catalogPending.addAll(AnimationCatalog.syncEntries(defaultCount))
                syncNextAnimation()
            }
            return
        }
        if (message.startsWith("OK CATALOG ")) {
            if (message.substringAfterLast(' ').toIntOrNull() == catalogAwaiting && catalogAwaiting != null) {
                mainHandler.removeCallbacks(catalogTimeout)
                catalogAwaiting = null
                syncNextAnimation()
            }
            return
        }
        if (message.startsWith("ERROR CATALOG")) {
            mainHandler.removeCallbacks(catalogTimeout)
            catalogPending.clear()
            catalogAwaiting = null
            onStatus(message)
            return
        }
        if (message.startsWith("EVENT ")) {
            Log.d("MegaCubeBle", "notification: ")
            onEvent(message.removePrefix("EVENT "))
        } else if (message.startsWith("OK ANIMATION ") || message.startsWith("STATUS ANIMATION ")) {
            val animationId = message.split(" ").getOrNull(2)?.toIntOrNull()
            musicSelected = animationId == 38 || animationId == 39
            if (!musicSelected) latestMusic = null
            if (musicSelected) onEvent("LIVE_START")
            if (animationId == 34 && message.startsWith("OK ")) onEvent("LIGHT_CHASE_START")
            onStatus(message)
        } else {
            if (message == "OK PLAYLIST" || message.startsWith("STATUS PLAYLIST ")) {
                musicSelected = false
                latestMusic = null
            }
            onStatus(message)
        }
    }

    private fun setState(value: ConnectionState) {
        state = value
        onConnectionChanged(value)
    }

    private fun clearWrites() {
        mainHandler.removeCallbacks(catalogTimeout)
        catalogPending.clear()
        catalogAwaiting = null
        catalogRequested = false
        mainHandler.removeCallbacks(writeTimeout)
        mainHandler.removeCallbacks(openingOnConnect)
        commands.clear()
        latestMusic = null
        writeInProgress = false
        musicSelected = false
        writeStartRetries = 0
        fastConnection = false
    }

    private fun pumpWrites() {
        if (writeInProgress || state != ConnectionState.CONNECTED) return
        val characteristic = commandCharacteristic ?: return
        val connection = gatt ?: return
        writingMusic = commands.isEmpty()
        val payload = if (!writingMusic) commands.first()
            else latestMusic ?: return
        val writeType = if (writingMusic) BluetoothGattCharacteristic.WRITE_TYPE_NO_RESPONSE
            else BluetoothGattCharacteristic.WRITE_TYPE_DEFAULT
        val accepted = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            connection.writeCharacteristic(characteristic, payload, writeType) == BluetoothStatusCodes.SUCCESS
        } else {
            characteristic.writeType = writeType
            characteristic.value = payload
            connection.writeCharacteristic(characteristic)
        }
        if (accepted) {
            writeStartRetries = 0
            if (writingMusic) latestMusic = null else commands.removeFirst()
            writeInProgress = true
            mainHandler.postDelayed(writeTimeout, 2000)
        } else {
            if (++writeStartRetries > 20) {
                onStatus("BLE is stalled; reconnect the cube")
                connection.disconnect()
                return
            }
            mainHandler.postDelayed({ pumpWrites() }, 50)
        }
    }

    private val scanCallback = object : ScanCallback() {
        override fun onScanResult(callbackType: Int, result: ScanResult) {
            val device = result.device
            val item = CubeDevice(device.name ?: "Mega Cube", device.address, result.rssi)
            devices[device.address] = item
            onDevicesChanged(devices.values.sortedByDescending { it.rssi })
        }

        override fun onScanFailed(errorCode: Int) {
            setState(ConnectionState.DISCONNECTED)
            onStatus("BLE scan failed: $errorCode")
        }
    }

    private val gattCallback = object : BluetoothGattCallback() {
        override fun onConnectionStateChange(gatt: BluetoothGatt, status: Int, newState: Int) {
            mainHandler.post {
            if (this@CubeBleManager.gatt !== gatt) return@post
            if (status == BluetoothGatt.GATT_SUCCESS &&
                newState == android.bluetooth.BluetoothProfile.STATE_CONNECTED
            ) {
                setState(ConnectionState.CONNECTING)
                gatt.discoverServices()
            } else {
                clearWrites()
                onEvent("STORM_STOP")
                onEvent("SINUS_STOP")
                onEvent("UNIVERSAL_STOP")
                onEvent("YMCA_STOP")
                commandCharacteristic = null
                setState(ConnectionState.DISCONNECTED)
                onStatus("Cube disconnected")
            }
            }
        }

        override fun onServicesDiscovered(gatt: BluetoothGatt, status: Int) {
            if (this@CubeBleManager.gatt !== gatt) return
            if (status != BluetoothGatt.GATT_SUCCESS) {
                onStatus("Cube service discovery failed")
                gatt.disconnect()
                return
            }
            val service: BluetoothGattService? = gatt.getService(SERVICE_UUID)
            commandCharacteristic = service?.getCharacteristic(COMMAND_UUID)
            val statusCharacteristic = service?.getCharacteristic(STATUS_UUID)
            if (commandCharacteristic == null || statusCharacteristic == null) {
                onStatus("Cube service not found")
                gatt.disconnect()
                return
            }

            val descriptor = statusCharacteristic.getDescriptor(CCCD_UUID)
            if (descriptor == null || !gatt.setCharacteristicNotification(statusCharacteristic, true)) {
                onStatus("Cube notifications unavailable")
                gatt.disconnect()
                return
            }
            val subscribed = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
                gatt.writeDescriptor(descriptor, BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE) == BluetoothStatusCodes.SUCCESS
            } else {
                descriptor.value = BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE
                gatt.writeDescriptor(descriptor)
            }
            if (!subscribed) {
                onStatus("Cube notification setup failed")
                gatt.disconnect()
            }
        }

        override fun onDescriptorWrite(gatt: BluetoothGatt, descriptor: BluetoothGattDescriptor, status: Int) {
            if (descriptor.uuid != CCCD_UUID) return
            mainHandler.post {
                if (this@CubeBleManager.gatt !== gatt) return@post
                if (status != BluetoothGatt.GATT_SUCCESS) {
                    onStatus("Cube notifications could not be enabled")
                    gatt.disconnect()
                    return@post
                }
                setState(ConnectionState.CONNECTED)
                onStatus("Connected to Mega Cube")
                catalogRequested = true
                send("CATALOG?")
                mainHandler.postDelayed(catalogTimeout, 3000)
                mainHandler.postDelayed({ configureAudioLatency() }, 600)
                mainHandler.postDelayed(openingOnConnect, 1000)
                pumpWrites()
            }
        }

        override fun onCharacteristicWrite(gatt: BluetoothGatt, characteristic: BluetoothGattCharacteristic, status: Int) {
            mainHandler.post {
                if (this@CubeBleManager.gatt !== gatt) return@post
                mainHandler.removeCallbacks(writeTimeout)
                writeInProgress = false
                if (status != BluetoothGatt.GATT_SUCCESS && !writingMusic)
                    onStatus("BLE command failed: $status")
                pumpWrites()
            }
        }

        @Deprecated("Deprecated in API 33")
        override fun onCharacteristicChanged(
            gatt: BluetoothGatt,
            characteristic: BluetoothGattCharacteristic,
        ) {
            val value = characteristic.value
            mainHandler.post { handleNotification(value) }
        }

        override fun onCharacteristicChanged(
            gatt: BluetoothGatt,
            characteristic: BluetoothGattCharacteristic,
            value: ByteArray,
        ) {
            mainHandler.post { handleNotification(value) }
        }
    }

    fun startScan() {
        stopScan()
        devices.clear()
        onDevicesChanged(emptyList())
        val filter = ScanFilter.Builder()
            .setServiceUuid(ParcelUuid(SERVICE_UUID))
            .build()
        val settings = ScanSettings.Builder()
            .setScanMode(ScanSettings.SCAN_MODE_LOW_LATENCY)
            .build()
        setState(ConnectionState.SCANNING)
        scanner?.startScan(listOf(filter), settings, scanCallback)
            ?: onStatus("Bluetooth is unavailable")
    }

    fun stopScan() {
        scanner?.stopScan(scanCallback)
        if (state == ConnectionState.SCANNING) setState(ConnectionState.DISCONNECTED)
    }

    fun connect(address: String) {
        stopScan()
        setState(ConnectionState.CONNECTING)
        val device = adapter?.getRemoteDevice(address) ?: return
        clearWrites()
        gatt?.close()
        gatt = device.connectGatt(appContext, false, gattCallback, BluetoothDevice.TRANSPORT_LE)
    }

    private fun configuredLatencyMs(): Int {
        val outputTypes = audioManager
            .getDevices(AudioManager.GET_DEVICES_OUTPUTS)
            .map { it.type }
        val bleAudio = Build.VERSION.SDK_INT >= Build.VERSION_CODES.S &&
            outputTypes.any { it == AudioDeviceInfo.TYPE_BLE_HEADSET ||
                it == AudioDeviceInfo.TYPE_BLE_SPEAKER }
        return AudioLatency.compensationMs(
            classicBluetooth = outputTypes.contains(AudioDeviceInfo.TYPE_BLUETOOTH_A2DP),
            bleAudio = bleAudio,
        )
    }

    private fun configureAudioLatency() {
        val latencyMs = configuredLatencyMs()
        Log.d("MegaCubeAudio", "audio route compensation=" + latencyMs + "ms")
        send("AUDIO_LATENCY " + latencyMs)
    }

    fun disconnect() {
        clearWrites()
        onEvent("LIGHT_CHASE_STOP")
        onEvent("MATRIX_STOP")
        onEvent("SAURON_STOP")
        onEvent("UNIVERSAL_STOP")
        onEvent("SINUS_STOP")
        onEvent("YMCA_STOP")
        gatt?.disconnect()
        gatt?.close()
        gatt = null
        commandCharacteristic = null
        setState(ConnectionState.DISCONNECTED)
    }

    fun selectAnimation(id: Int) {
        // This is a temporary overlay: keep the current soundtrack and live input.
        if (id == 40 || id == AnimationCatalog.WALL_DOUBLE_FLASH ||
            id == AnimationCatalog.EDGE_COLOR_SEQUENCE || id == AnimationCatalog.EDGE_COLOR_SEQUENCE2) {
            send("ANIMATION " + id)
            return
        }
        mainHandler.removeCallbacks(openingOnConnect)
        musicSelected = id == 38 || id == 39
        latestMusic = null
        if (id != 34) onEvent("LIGHT_CHASE_STOP")
        onEvent(if (id == 19) "MATRIX_START" else "MATRIX_STOP")
        onEvent(if (id == 27) "SAURON_START" else "SAURON_STOP")
        if (id != 36) onEvent("UNIVERSAL_STOP")
        if (id != 12) onEvent("SINUS_STOP")
        if (id != 50) onEvent("YMCA_STOP")
        if (id == 30 || id == 31) {
            onEvent("OPENING")
            mainHandler.postDelayed({ send("ANIMATION " + id) }, 650)
        } else {
            send("ANIMATION " + id)
        }
    }

    fun send(command: String): Boolean {
        if (commandCharacteristic == null || state != ConnectionState.CONNECTED) return false
        if (command == "PLAYLIST") {
            onEvent("YMCA_STOP")
            musicSelected = false
            latestMusic = null
        }
        val payload = command.toByteArray(StandardCharsets.UTF_8)
        if (commands.size >= 32) {
            onStatus("BLE command queue is full")
            return false
        }
        commands.addLast(payload)
        pumpWrites()
        return true
    }

    private fun syncNextAnimation() {
        val next = catalogPending.pollFirst() ?: return
        val packets = if (next.name.isEmpty()) listOf(AnimationCatalog.deletePacket(next.id))
            else AnimationCatalog.packets(next)
        if (commands.size + packets.size > 32) {
            catalogPending.clear()
            onStatus("Animation catalog sync deferred; reconnect to retry")
            return
        }
        catalogAwaiting = next.id
        packets.forEach(commands::addLast)
        mainHandler.postDelayed(catalogTimeout, 3000)
        pumpWrites()
    }

    fun sendMusic(features: MusicFeatures) {
        mainHandler.post {
            if (state != ConnectionState.CONNECTED || !musicSelected) return@post
            if (!fastConnection) {
                gatt?.requestConnectionPriority(BluetoothGatt.CONNECTION_PRIORITY_HIGH)
                fastConnection = true
            }
            latestMusic = features.packet(musicSequence++)
            pumpWrites()
        }
    }

    fun stopMusic() {
        mainHandler.post {
            latestMusic = null
            if (fastConnection) gatt?.requestConnectionPriority(BluetoothGatt.CONNECTION_PRIORITY_BALANCED)
            fastConnection = false
        }
    }

    fun close() {
        audioManager.unregisterAudioDeviceCallback(audioDeviceCallback)
        disconnect()
    }
}
