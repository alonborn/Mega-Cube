package com.megacube.controller

import org.jtransforms.fft.FloatFFT_1D
import kotlin.math.PI
import kotlin.math.cos
import kotlin.math.log10
import kotlin.math.max
import kotlin.math.roundToInt
import kotlin.math.sqrt

data class MusicFeatures(val level: Int, val onset: Int, val beat: Int, val bands: IntArray) {
    fun packet(sequence: Int): ByteArray {
        require(bands.size == 8)
        val bytes = ByteArray(16)
        bytes[0] = 'M'.code.toByte()
        bytes[1] = 'U'.code.toByte()
        bytes[2] = 1
        bytes[3] = sequence.toByte()
        bytes[4] = level.coerceIn(0, 255).toByte()
        bytes[5] = onset.coerceIn(0, 255).toByte()
        bytes[6] = beat.toByte()
        bands.forEachIndexed { i, value -> bytes[7 + i] = value.coerceIn(0, 255).toByte() }
        bytes[15] = bytes.take(15).fold(0) { sum, value -> sum xor (value.toInt() and 255) }.toByte()
        return bytes
    }
}

class MusicAnalysis {
    companion object {
        const val SAMPLE_RATE = 48000
        const val FRAME_SIZE = 1024
    }

    private val fft = FloatFFT_1D(FRAME_SIZE.toLong())
    private val spectrum = FloatArray(FRAME_SIZE)
    private val window = FloatArray(FRAME_SIZE) {
        (0.5 - 0.5 * cos(2.0 * PI * it / (FRAME_SIZE - 1))).toFloat()
    }
    private val previous = FloatArray(FRAME_SIZE / 2)
    private val edges = intArrayOf(43, 86, 172, 345, 690, 1380, 2760, 5520, 10000)
    private var averageFlux = 0f
    private var frames = 0
    private var lastBeatAt = -1000L
    private var beatCounter = 0
    private var onsetStrength = 0

    private fun intensity(amplitude: Float, gain: Float): Int {
        val db = 20f * log10(max(0.000001f, amplitude * gain))
        return (((db + 48f) / 39f).coerceIn(0f, 1f) * 255f).roundToInt()
    }

    fun analyze(samples: ShortArray, gain: Float, nowMs: Long): MusicFeatures {
        require(samples.size == FRAME_SIZE)
        val mean = samples.sumOf { it.toDouble() }.toFloat() / FRAME_SIZE
        var energy = 0f
        for (i in samples.indices) {
            val value = (samples[i] - mean) / 32768f
            energy += value * value
            spectrum[i] = value * window[i]
        }
        val level = intensity(sqrt(energy / FRAME_SIZE), gain)
        fft.realForward(spectrum)
        val bandPower = FloatArray(8)
        var flux = 0f
        for (bin in 1 until FRAME_SIZE / 2) {
            val real = spectrum[2 * bin]
            val imaginary = spectrum[2 * bin + 1]
            val magnitude = sqrt(real * real + imaginary * imaginary) * 4f / FRAME_SIZE
            val frequency = bin * SAMPLE_RATE.toFloat() / FRAME_SIZE
            if (frequency in 43f..6000f) flux += max(0f, magnitude - previous[bin])
            previous[bin] = magnitude
            for (band in 0..7) {
                if (frequency >= edges[band] && frequency < edges[band + 1]) {
                    bandPower[band] += magnitude * magnitude
                    break
                }
            }
        }
        flux *= gain
        val threshold = max(0.015f, averageFlux * 1.8f)
        val detected = frames >= 12 && level > 35 && flux > threshold && nowMs - lastBeatAt >= 250
        if (detected) {
            lastBeatAt = nowMs
            beatCounter = (beatCounter + 1) and 255
            onsetStrength = ((flux / max(threshold, 0.015f)).coerceIn(0f, 2f) * 127.5f).roundToInt()
        }
        averageFlux = averageFlux * 0.92f + flux * 0.08f
        ++frames
        val bands = IntArray(8) { intensity(sqrt(bandPower[it]), gain) }
        return MusicFeatures(level, onsetStrength, beatCounter, bands)
    }
}
