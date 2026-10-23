package com.megacube.controller

import org.junit.Assert.*
import org.junit.Test
import kotlin.math.PI
import kotlin.math.sin

class MusicAnalysisTest {
    private fun tone(hz: Double) = ShortArray(MusicAnalysis.FRAME_SIZE) {
        (12000 * sin(2 * PI * hz * it / MusicAnalysis.SAMPLE_RATE)).toInt().toShort()
    }

    @Test fun silenceStaysDark() {
        val analyzer = MusicAnalysis()
        repeat(30) {
            val frame = analyzer.analyze(ShortArray(MusicAnalysis.FRAME_SIZE), 1f, it * 47L)
            assertEquals(0, frame.level)
            assertEquals(0, frame.beat)
            assertTrue(frame.bands.all { band -> band == 0 })
        }
    }

    @Test fun tonesReachTheirFrequencyBand() {
        for ((hz, expected) in listOf(65.0 to 0, 1000.0 to 4, 7000.0 to 7)) {
            val frame = MusicAnalysis().analyze(tone(hz), 1f, 0)
            assertEquals(expected, frame.bands.indices.maxBy { frame.bands[it] })
        }
    }

    @Test fun onsetHasRefractoryPeriod() {
        val analyzer = MusicAnalysis()
        repeat(15) { analyzer.analyze(ShortArray(MusicAnalysis.FRAME_SIZE), 1f, it * 47L) }
        val first = analyzer.analyze(tone(1000.0), 1f, 750)
        assertEquals(1, first.beat)
        assertTrue(first.onset > 0)
        val next = analyzer.analyze(ShortArray(MusicAnalysis.FRAME_SIZE), 1f, 800)
        assertEquals(first.onset, next.onset)
        assertEquals(first.beat, next.beat)
        assertEquals(1, analyzer.analyze(tone(1000.0), 1f, 850).beat)
    }

    @Test fun packetFitsDefaultBleMtuAndKeepsUnsignedBytes() {
        val packet = MusicFeatures(255, 128, 254, intArrayOf(0, 32, 64, 96, 128, 160, 192, 255)).packet(255)
        assertEquals(16, packet.size)
        assertEquals(255, packet[3].toInt() and 255)
        assertEquals(255, packet[4].toInt() and 255)
        val uartHex = packet.sliceArray(3..14).joinToString("") { "%02X".format(it.toInt() and 255) }
        assertEquals("FFFF80FE0020406080A0C0FF", uartHex)
        val checksum: Int = packet.fold(0) { sum, byte -> sum xor (byte.toInt() and 255) }
        assertEquals(0, checksum)
    }
}
