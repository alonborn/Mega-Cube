package com.megacube.controller

import android.annotation.SuppressLint
import android.media.AudioFormat
import android.media.AudioRecord
import android.media.MediaRecorder
import android.os.Handler
import android.os.Build
import android.os.Looper
import android.os.Process
import android.os.SystemClock

class LiveMusicMicrophone(
    private val onFrame: (MusicFeatures) -> Unit,
    private val onStatus: (String) -> Unit,
) {
    private val mainHandler = Handler(Looper.getMainLooper())
    @Volatile var sensitivity = 1f
    private class Capture(val recorder: AudioRecord) {
        @Volatile var running = true
        var worker: Thread? = null
    }
    private var capture: Capture? = null

    @SuppressLint("MissingPermission")
    fun start() {
        if (capture?.worker?.isAlive == true) return
        var input: AudioRecord? = null
        try {
            val minimum = AudioRecord.getMinBufferSize(MusicAnalysis.SAMPLE_RATE,
                AudioFormat.CHANNEL_IN_MONO, AudioFormat.ENCODING_PCM_16BIT)
            check(minimum > 0) { "Microphone format unavailable" }
            fun openInput(source: Int): AudioRecord {
                val candidate = AudioRecord.Builder()
                .setAudioSource(source)
                .setAudioFormat(AudioFormat.Builder()
                    .setSampleRate(MusicAnalysis.SAMPLE_RATE)
                    .setChannelMask(AudioFormat.CHANNEL_IN_MONO)
                    .setEncoding(AudioFormat.ENCODING_PCM_16BIT).build())
                .setBufferSizeInBytes(maxOf(minimum, MusicAnalysis.FRAME_SIZE * 2))
                .build()
                try {
                    check(candidate.state == AudioRecord.STATE_INITIALIZED) { "Microphone unavailable" }
                    candidate.startRecording()
                    return candidate
                } catch (error: Exception) {
                    candidate.release()
                    throw error
                }
            }
            val recording = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
                try { openInput(MediaRecorder.AudioSource.VOICE_PERFORMANCE) }
                catch (_: Exception) { openInput(MediaRecorder.AudioSource.VOICE_RECOGNITION) }
            } else openInput(MediaRecorder.AudioSource.VOICE_RECOGNITION)
            input = recording
            val session = Capture(recording)
            capture = session
            onStatus("Listening")
            session.worker = Thread({
                Process.setThreadPriority(Process.THREAD_PRIORITY_AUDIO)
                try {
                    val analysis = MusicAnalysis()
                    val samples = ShortArray(MusicAnalysis.FRAME_SIZE)
                    var offset = 0
                    while (session.running) {
                        val count = recording.read(samples, offset, samples.size - offset, AudioRecord.READ_BLOCKING)
                        if (!session.running) break
                        check(count > 0) { "Microphone read failed: $count" }
                        offset += count
                        if (offset == samples.size) {
                            val features = analysis.analyze(samples, sensitivity, SystemClock.elapsedRealtime())
                            if (session.running) onFrame(features)
                            offset = 0
                        }
                    }
                } catch (error: Exception) {
                    if (session.running) mainHandler.post {
                        if (capture === session) onStatus(error.message ?: "Microphone error")
                    }
                } finally {
                    session.running = false
                    recording.release()
                }
            }, "CubeMusicAnalysis").also { it.start() }
        } catch (error: Exception) {
            input?.release()
            capture = null
            onStatus(error.message ?: "Microphone unavailable")
        }
    }

    fun stop() {
        val session = capture ?: return
        capture = null
        session.running = false
        try { session.recorder.stop() } catch (_: IllegalStateException) { }
        session.worker?.join(300)
    }
}
