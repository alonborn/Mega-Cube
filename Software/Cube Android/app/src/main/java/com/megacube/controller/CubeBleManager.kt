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
import android.bluetooth.le.BluetoothLeScanner
import android.bluetooth.le.ScanCallback
import android.bluetooth.le.ScanFilter
import android.bluetooth.le.ScanResult
import android.bluetooth.le.ScanSettings
import android.content.Context
import android.os.ParcelUuid
import java.nio.charset.StandardCharsets
import java.util.UUID

data class CubeDevice(val name: String, val address: String, val rssi: Int)

enum class ConnectionState { DISCONNECTED, SCANNING, CONNECTING, CONNECTED }

@SuppressLint("MissingPermission")
class CubeBleManager(
    context: Context,
    private val onDevicesChanged: (List<CubeDevice>) -> Unit,
    private val onConnectionChanged: (ConnectionState) -> Unit,
    private val onStatus: (String) -> Unit,
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

    private var gatt: BluetoothGatt? = null
    private var commandCharacteristic: BluetoothGattCharacteristic? = null
    private var state = ConnectionState.DISCONNECTED

    private fun setState(value: ConnectionState) {
        state = value
        onConnectionChanged(value)
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
            if (status == BluetoothGatt.GATT_SUCCESS &&
                newState == android.bluetooth.BluetoothProfile.STATE_CONNECTED
            ) {
                setState(ConnectionState.CONNECTING)
                gatt.discoverServices()
            } else {
                commandCharacteristic = null
                setState(ConnectionState.DISCONNECTED)
                onStatus("Cube disconnected")
            }
        }

        override fun onServicesDiscovered(gatt: BluetoothGatt, status: Int) {
            val service: BluetoothGattService? = gatt.getService(SERVICE_UUID)
            commandCharacteristic = service?.getCharacteristic(COMMAND_UUID)
            val statusCharacteristic = service?.getCharacteristic(STATUS_UUID)
            if (commandCharacteristic == null || statusCharacteristic == null) {
                onStatus("Cube service not found")
                gatt.disconnect()
                return
            }

            gatt.setCharacteristicNotification(statusCharacteristic, true)
            statusCharacteristic.getDescriptor(CCCD_UUID)?.let { descriptor ->
                descriptor.value = BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE
                gatt.writeDescriptor(descriptor)
            }
            setState(ConnectionState.CONNECTED)
            onStatus("Connected to Mega Cube")
        }

        @Deprecated("Deprecated in API 33")
        override fun onCharacteristicChanged(
            gatt: BluetoothGatt,
            characteristic: BluetoothGattCharacteristic,
        ) {
            onStatus(characteristic.value.toString(StandardCharsets.UTF_8))
        }

        override fun onCharacteristicChanged(
            gatt: BluetoothGatt,
            characteristic: BluetoothGattCharacteristic,
            value: ByteArray,
        ) {
            onStatus(value.toString(StandardCharsets.UTF_8))
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
        gatt?.close()
        gatt = device.connectGatt(appContext, false, gattCallback, BluetoothDevice.TRANSPORT_LE)
    }

    fun disconnect() {
        gatt?.disconnect()
        gatt?.close()
        gatt = null
        commandCharacteristic = null
        setState(ConnectionState.DISCONNECTED)
    }

    fun send(command: String): Boolean {
        val characteristic = commandCharacteristic ?: return false
        val payload = command.toByteArray(StandardCharsets.UTF_8)
        characteristic.writeType = BluetoothGattCharacteristic.WRITE_TYPE_DEFAULT
        characteristic.value = payload
        return gatt?.writeCharacteristic(characteristic) == true
    }
}
