package com.megacube.controller

import org.junit.Assert.assertEquals
import org.junit.Test

class AudioLatencyTest {
    @Test fun phoneAndNonBluetoothOutputsKeepOriginalTiming() {
        assertEquals(0, AudioLatency.compensationMs(false, false))
    }

    @Test fun bluetoothSpeakerAddsOneHundredMilliseconds() {
        assertEquals(350, AudioLatency.compensationMs(true, false))
    }

    @Test fun bleAudioAlsoAddsOneHundredMilliseconds() {
        assertEquals(280, AudioLatency.compensationMs(false, true))
    }

    @Test fun classicBluetoothKeepsPriorityWhenBothAreReported() {
        assertEquals(350, AudioLatency.compensationMs(true, true))
    }
}
