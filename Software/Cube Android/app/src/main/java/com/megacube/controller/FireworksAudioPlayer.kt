package com.megacube.controller

import android.content.Context
import android.media.AudioAttributes
import android.media.SoundPool
import android.util.Log

class FireworksAudioPlayer(context: Context) {
    companion object {
        private const val TAG = "MegaCubeAudio"
        private const val LAUNCH = "LAUNCH"
        private const val EXPLOSION = "EXPLOSION"
    }

    private val soundPool = SoundPool.Builder()
        .setMaxStreams(8)
        .setAudioAttributes(
            AudioAttributes.Builder()
                .setUsage(AudioAttributes.USAGE_GAME)
                .setContentType(AudioAttributes.CONTENT_TYPE_SONIFICATION)
                .build()
        )
        .build()

    private val soundTypes = mutableMapOf<Int, String>()
    private val loadedSounds = mutableMapOf<String, MutableList<Int>>()
    private val pendingEvents = ArrayDeque<String>()

    init {
        soundPool.setOnLoadCompleteListener { _, sampleId, status ->
            val type = soundTypes[sampleId]
            Log.d(TAG, "load sample=$sampleId type=$type status=$status")
            if (status == 0 && type != null) {
                loadedSounds.getOrPut(type) { mutableListOf() }.add(sampleId)
                val pending = pendingEvents.firstOrNull { if (it.startsWith(EXPLOSION)) EXPLOSION == type else it == type }
                if (pending != null) {
                    pendingEvents.remove(pending)
                    playEvent(pending)
                }
            }
        }

        load(context, LAUNCH, listOf(
            R.raw.launch_01,
            R.raw.launch_02,
            R.raw.launch_03,
            R.raw.launch_04,
            R.raw.launch_05,
        ))
        load(context, EXPLOSION, listOf(
            R.raw.explosion_01,
            R.raw.explosion_03,
            R.raw.explosion_04,
            R.raw.explosion_05,
            R.raw.explosion_07,
            R.raw.explosion_08,
        ))
    }

    private fun load(context: Context, type: String, resources: List<Int>) {
        resources.forEach { resource ->
            val sampleId = soundPool.load(context, resource, 1)
            soundTypes[sampleId] = type
        }
    }

    @Synchronized
    fun playEvent(event: String) {
        val type = if (event.startsWith(EXPLOSION)) EXPLOSION else event
        val sounds = loadedSounds[type].orEmpty()
        if (sounds.isEmpty()) {
            if (pendingEvents.size < 8) pendingEvents.addLast(event)
            Log.w(TAG, "queued event=$event; no loaded sample yet")
            return
        }
        val count = if (type == EXPLOSION)
            event.substringAfter("_", "1").toIntOrNull()?.coerceIn(1, 4) ?: 1
        else 1
        sounds.shuffled().take(count).forEach { sampleId ->
            val streamId = soundPool.play(sampleId, 1f, 1f, 1, 0, 1f)
            Log.d(TAG, "play event=$event sample=$sampleId stream=$streamId")
        }
    }

    fun release() {
        soundPool.release()
    }
}
