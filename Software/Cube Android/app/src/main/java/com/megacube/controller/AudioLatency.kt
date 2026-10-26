package com.megacube.controller

internal object AudioLatency {
    private const val BLUETOOTH_EXTRA_MS = 100

    fun compensationMs(classicBluetooth: Boolean, bleAudio: Boolean): Int = when {
        classicBluetooth -> 250 + BLUETOOTH_EXTRA_MS
        bleAudio -> 180 + BLUETOOTH_EXTRA_MS
        else -> 0
    }
}
