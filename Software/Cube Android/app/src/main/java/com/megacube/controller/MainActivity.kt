package com.megacube.controller

import android.Manifest
import android.content.pm.PackageManager
import android.os.Handler
import android.os.Looper
import android.os.Build
import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.rememberLauncherForActivityResult
import androidx.activity.compose.setContent
import androidx.activity.result.contract.ActivityResultContracts
import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.rounded.Bluetooth
import androidx.compose.material.icons.rounded.Refresh
import androidx.compose.material3.Button
import androidx.compose.material3.ButtonDefaults
import androidx.compose.material3.Card
import androidx.compose.material3.CardDefaults
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Slider
import androidx.compose.material3.Switch
import androidx.compose.material3.LinearProgressIndicator
import androidx.compose.material3.Text
import androidx.compose.material3.TopAppBar
import androidx.compose.runtime.Composable
import androidx.compose.runtime.DisposableEffect
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.mutableIntStateOf
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableFloatStateOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.core.content.ContextCompat
import androidx.lifecycle.Lifecycle
import androidx.lifecycle.LifecycleEventObserver

private val animations = AnimationCatalog.choices

class MainActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContent { MegaCubeApp() }
    }
}

@Composable
private fun MegaCubeApp() {
    var devices by remember { mutableStateOf(emptyList<CubeDevice>()) }
    var connection by remember { mutableStateOf(ConnectionState.DISCONNECTED) }
    var status by remember { mutableStateOf("Ready") }
    val context = androidx.compose.ui.platform.LocalContext.current
    val mainHandler = remember { Handler(Looper.getMainLooper()) }
    var activeAnimation by remember { mutableIntStateOf(-1) }
    var foreground by remember { mutableStateOf(true) }
    var liveEnabled by remember { mutableStateOf(true) }
    var sensitivity by remember { mutableFloatStateOf(1f) }
    var liveLevel by remember { mutableFloatStateOf(0f) }
    var liveStatus by remember { mutableStateOf("Microphone off") }
    var microphonePermission by remember { mutableStateOf(
        ContextCompat.checkSelfPermission(context, Manifest.permission.RECORD_AUDIO) == PackageManager.PERMISSION_GRANTED
    ) }
    var pendingLiveAnimation by remember { mutableStateOf<Int?>(null) }
    val fireworksAudio = remember { FireworksAudioPlayer(context) }
    val manager = remember {
        CubeBleManager(
            context,
            onDevicesChanged = { value -> mainHandler.post { devices = value } },
            onConnectionChanged = { value -> mainHandler.post { connection = value } },
            onStatus = { message -> mainHandler.post {
                status = message
                when {
                    message.startsWith("OK ANIMATION ") -> activeAnimation = message.substringAfterLast(" ").toIntOrNull() ?: -1
                    message.startsWith("STATUS ANIMATION ") -> activeAnimation = message.split(" ").getOrNull(2)?.toIntOrNull() ?: -1
                    message == "OK PLAYLIST" || message.startsWith("STATUS PLAYLIST ") -> activeAnimation = -1
                }
            } },
            onEvent = { event -> mainHandler.post { fireworksAudio.playEvent(event) } },
        )
    }
    val microphone = remember {
        var frames = 0
        LiveMusicMicrophone(
            onFrame = { features ->
                manager.sendMusic(features)
                if (++frames % 4 == 0) mainHandler.post { liveLevel = features.level / 255f }
            },
            onStatus = { liveStatus = it },
        )
    }
    val microphoneLauncher = rememberLauncherForActivityResult(
        ActivityResultContracts.RequestPermission()
    ) { granted ->
        microphonePermission = granted
        liveEnabled = granted
        if (granted) pendingLiveAnimation?.let(manager::selectAnimation)
        else status = "Microphone permission is required"
        pendingLiveAnimation = null
    }

    LaunchedEffect(connection, activeAnimation, foreground, liveEnabled, microphonePermission) {
        if (connection == ConnectionState.CONNECTED && activeAnimation in 38..39 &&
            foreground && liveEnabled && microphonePermission) {
            microphone.start()
        } else {
            microphone.stop()
            manager.stopMusic()
            liveLevel = 0f
            liveStatus = "Microphone off"
        }
    }
    LaunchedEffect(sensitivity) { microphone.sensitivity = sensitivity }

    val permissions = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
        arrayOf(Manifest.permission.BLUETOOTH_SCAN, Manifest.permission.BLUETOOTH_CONNECT)
    } else {
        arrayOf(Manifest.permission.ACCESS_FINE_LOCATION)
    }
    val permissionLauncher = rememberLauncherForActivityResult(
        ActivityResultContracts.RequestMultiplePermissions()
    ) { result ->
        if (result.values.all { it }) manager.startScan()
        else status = "Bluetooth permission is required"
    }

    DisposableEffect(Unit) {
        val lifecycle = (context as ComponentActivity).lifecycle
        val observer = LifecycleEventObserver { _, event ->
            if (event == Lifecycle.Event.ON_START) foreground = true
            if (event == Lifecycle.Event.ON_STOP) foreground = false
        }
        lifecycle.addObserver(observer)
        onDispose {
            lifecycle.removeObserver(observer)
            microphone.stop()
            manager.close()
            fireworksAudio.release()
        }
    }

    MaterialTheme {
        if (connection == ConnectionState.CONNECTED) {
            ControlScreen(status, manager,
                onSelect = { id ->
                    if (id in 38..39) {
                        liveEnabled = true
                        if (!microphonePermission) {
                            pendingLiveAnimation = id
                            microphoneLauncher.launch(Manifest.permission.RECORD_AUDIO)
                        } else manager.selectAnimation(id)
                    } else manager.selectAnimation(id)
                },
                live = activeAnimation in 38..39,
                liveEnabled = liveEnabled,
                liveStatus = liveStatus,
                liveLevel = liveLevel,
                sensitivity = sensitivity,
                onSensitivity = { sensitivity = it },
                onLiveEnabled = { enabled ->
                    if (enabled && !microphonePermission)
                        microphoneLauncher.launch(Manifest.permission.RECORD_AUDIO)
                    else liveEnabled = enabled
                },
            )
        } else {
            DeviceScreen(
                devices = devices,
                state = connection,
                status = status,
                onScan = { permissionLauncher.launch(permissions) },
                onConnect = manager::connect,
            )
        }
    }
}

@OptIn(ExperimentalMaterial3Api::class)
@Composable
private fun DeviceScreen(
    devices: List<CubeDevice>,
    state: ConnectionState,
    status: String,
    onScan: () -> Unit,
    onConnect: (String) -> Unit,
) {
    Scaffold(
        topBar = {
            TopAppBar(
                title = { Text("Mega Cube", fontWeight = FontWeight.Bold) },
                actions = {
                    IconButton(onClick = onScan) {
                        Icon(Icons.Rounded.Refresh, contentDescription = "Scan")
                    }
                },
            )
        },
    ) { padding ->
        Column(
            modifier = Modifier.fillMaxSize().padding(padding).padding(20.dp),
            verticalArrangement = Arrangement.spacedBy(16.dp),
        ) {
            Row(verticalAlignment = Alignment.CenterVertically) {
                Box(
                    Modifier.size(44.dp).background(Color(0xFFE32636), RoundedCornerShape(6.dp)),
                    contentAlignment = Alignment.Center,
                ) {
                    Icon(Icons.Rounded.Bluetooth, contentDescription = null, tint = Color.White)
                }
                Column(Modifier.padding(start = 14.dp)) {
                    Text(
                        if (state == ConnectionState.SCANNING) "Searching for cubes" else "Nearby cubes",
                        style = MaterialTheme.typography.titleLarge,
                    )
                    Text(status, color = Color.Gray)
                }
            }

            if (devices.isEmpty()) {
                Spacer(Modifier.weight(1f))
                Button(onClick = onScan, modifier = Modifier.fillMaxWidth()) {
                    Text("Scan for Mega Cube")
                }
                Spacer(Modifier.weight(1f))
            } else {
                LazyColumn(verticalArrangement = Arrangement.spacedBy(8.dp)) {
                    items(devices, key = { it.address }) { device ->
                        Card(
                            modifier = Modifier.fillMaxWidth().clickable { onConnect(device.address) },
                            colors = CardDefaults.cardColors(containerColor = Color(0xFFF0F0ED)),
                            shape = RoundedCornerShape(6.dp),
                        ) {
                            Row(
                                Modifier.fillMaxWidth().padding(16.dp),
                                horizontalArrangement = Arrangement.SpaceBetween,
                            ) {
                                Column {
                                    Text(device.name, fontWeight = FontWeight.SemiBold)
                                    Text(device.address, color = Color.Gray)
                                }
                                Text("${device.rssi} dBm", color = Color.Gray)
                            }
                        }
                    }
                }
            }
        }
    }
}

@OptIn(ExperimentalMaterial3Api::class)
@Composable
private fun ControlScreen(
    status: String,
    manager: CubeBleManager,
    onSelect: (Int) -> Unit,
    live: Boolean,
    liveEnabled: Boolean,
    liveStatus: String,
    liveLevel: Float,
    sensitivity: Float,
    onSensitivity: (Float) -> Unit,
    onLiveEnabled: (Boolean) -> Unit,
) {
    var brightness by remember { mutableFloatStateOf(180f) }

    Scaffold(topBar = { TopAppBar(title = { Text("Mega Cube") }) }) { padding ->
        Column(
            Modifier.fillMaxSize().padding(padding).padding(horizontal = 20.dp),
            verticalArrangement = Arrangement.spacedBy(14.dp),
        ) {
            Text("CONNECTED", color = Color(0xFF18864B), fontWeight = FontWeight.Bold)
            Text(status, color = Color.Gray)

            if (live) {
                Row(Modifier.fillMaxWidth(), horizontalArrangement = Arrangement.SpaceBetween,
                    verticalAlignment = Alignment.CenterVertically) {
                    Column {
                        Text("Live microphone", fontWeight = FontWeight.Medium)
                        Text(liveStatus, color = Color.Gray)
                    }
                    Switch(checked = liveEnabled, onCheckedChange = onLiveEnabled)
                }
                LinearProgressIndicator(progress = { liveLevel }, modifier = Modifier.fillMaxWidth())
                Text("Sensitivity ${"%.1f".format(sensitivity)}x")
                Slider(value = sensitivity, onValueChange = onSensitivity, valueRange = 0.5f..6f)
            }

            Text("Animation", style = MaterialTheme.typography.titleLarge)
            LazyColumn(modifier = Modifier.weight(1f), verticalArrangement = Arrangement.spacedBy(7.dp)) {
                item {
                    Button(
                        onClick = { manager.send("PLAYLIST") },
                        modifier = Modifier.fillMaxWidth(),
                        colors = ButtonDefaults.buttonColors(containerColor = Color(0xFFE32636)),
                        shape = RoundedCornerShape(6.dp),
                    ) { Text("Play playlist") }
                }
                items(animations, key = { it.id }) { animation ->
                    Card(
                        modifier = Modifier.fillMaxWidth().clickable {
                            onSelect(animation.id)
                        },
                        colors = CardDefaults.cardColors(containerColor = Color(0xFFF0F0ED)),
                        shape = RoundedCornerShape(6.dp),
                    ) {
                        Row(
                            Modifier.fillMaxWidth().padding(15.dp),
                            horizontalArrangement = Arrangement.SpaceBetween,
                        ) {
                            Text(animation.name, fontWeight = FontWeight.Medium)
                            Text("${animation.id}", color = Color.Gray)
                        }
                    }
                }
            }

            Text("Brightness ${brightness.toInt()}")
            Slider(
                value = brightness,
                onValueChange = { brightness = it },
                onValueChangeFinished = { manager.send("BRIGHTNESS ${brightness.toInt()}") },
                valueRange = 0f..255f,
            )
            Spacer(Modifier.height(8.dp))
        }
    }
}

