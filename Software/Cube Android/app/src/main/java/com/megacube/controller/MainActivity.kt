package com.megacube.controller

import android.Manifest
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
import androidx.compose.material3.Text
import androidx.compose.material3.TopAppBar
import androidx.compose.runtime.Composable
import androidx.compose.runtime.DisposableEffect
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

data class AnimationChoice(val id: Int, val name: String)

private val animations = listOf(
    AnimationChoice(3, "Atoms"),
    AnimationChoice(5, "Fireworks"),
    AnimationChoice(6, "Helix"),
    AnimationChoice(12, "Sinus"),
    AnimationChoice(14, "Starfield"),
    AnimationChoice(19, "The Matrix"),
    AnimationChoice(20, "Spotted Sphere"),
    AnimationChoice(21, "Supernova"),
    AnimationChoice(22, "Aurora"),
    AnimationChoice(23, "Black Hole"),
    AnimationChoice(24, "Metaballs"),
    AnimationChoice(25, "Electric Storm"),
    AnimationChoice(26, "DNA Tunnel"),
    AnimationChoice(27, "Eye of Sauron"),
    AnimationChoice(28, "White Test"),
)

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
    val manager = remember {
        CubeBleManager(
            context,
            onDevicesChanged = { devices = it },
            onConnectionChanged = { connection = it },
            onStatus = { status = it },
        )
    }

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
        onDispose { manager.disconnect() }
    }

    MaterialTheme {
        if (connection == ConnectionState.CONNECTED) {
            ControlScreen(status, manager)
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
private fun ControlScreen(status: String, manager: CubeBleManager) {
    var brightness by remember { mutableFloatStateOf(180f) }

    Scaffold(topBar = { TopAppBar(title = { Text("Mega Cube") }) }) { padding ->
        Column(
            Modifier.fillMaxSize().padding(padding).padding(horizontal = 20.dp),
            verticalArrangement = Arrangement.spacedBy(14.dp),
        ) {
            Text("CONNECTED", color = Color(0xFF18864B), fontWeight = FontWeight.Bold)
            Text(status, color = Color.Gray)

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
                            manager.send("ANIMATION ${animation.id}")
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

