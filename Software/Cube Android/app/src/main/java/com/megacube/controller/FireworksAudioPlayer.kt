package com.megacube.controller

import android.content.Context
import android.media.AudioAttributes
import android.media.MediaPlayer
import android.media.SoundPool
import android.util.Log

class FireworksAudioPlayer(context: Context) {
    companion object {
        private const val TAG = "MegaCubeAudio"
        private const val LAUNCH = "LAUNCH"
        private const val EXPLOSION = "EXPLOSION"
        private const val THUNDER = "THUNDER"
        private const val THUNDER_BOOM = "THUNDER_BOOM"
    }

    private val soundPool = SoundPool.Builder()
        .setMaxStreams(10)
        .setAudioAttributes(
            AudioAttributes.Builder()
                .setUsage(AudioAttributes.USAGE_GAME)
                .setContentType(AudioAttributes.CONTENT_TYPE_SONIFICATION)
                .build()
        )
        .build()

    private val openingPlayer = MediaPlayer.create(context, R.raw.opening_intro).apply {
        setVolume(1.0f, 1.0f)
    }
    private val matrixPlayer = MediaPlayer.create(context, R.raw.matrix_2).apply {
        isLooping = true
        setVolume(1.0f, 1.0f)
    }
    private val sauronPlayer = MediaPlayer.create(context, R.raw.eye_of_sauron).apply {
        isLooping = true
        setVolume(1.0f, 1.0f)
    }
    private val rainPlayer = MediaPlayer.create(context, R.raw.storm_rain_loop).apply {
        isLooping = true
        setVolume(0.22f, 0.22f)
    }
    private val soundTypes = mutableMapOf<Int, String>()
    private val loadedSounds = mutableMapOf<String, MutableList<Int>>()
    private val pendingEvents = ArrayDeque<String>()

    init {
        soundPool.setOnLoadCompleteListener { _, sampleId, status ->
            val type = soundTypes[sampleId]
            Log.d(TAG, "load sample=$sampleId type=$type status=$status")
            if (status == 0 && type != null) {
                loadedSounds.getOrPut(type) { mutableListOf() }.add(sampleId)
                val pending = pendingEvents.firstOrNull { eventType(it) == type }
                if (pending != null) {
                    pendingEvents.remove(pending)
                    playEvent(pending)
                }
            }
        }

        load(context, LAUNCH, listOf(
            R.raw.launch_01, R.raw.launch_02, R.raw.launch_03,
            R.raw.launch_04, R.raw.launch_05,
        ))
        load(context, EXPLOSION, listOf(
            R.raw.explosion_01, R.raw.explosion_03, R.raw.explosion_04,
            R.raw.explosion_05, R.raw.explosion_07, R.raw.explosion_08,
        ))
        load(context, THUNDER, listOf(
            R.raw.storm_thunder_1, R.raw.storm_thunder_2,
            R.raw.storm_thunder_3, R.raw.storm_thunder_4,
            R.raw.storm_thunder_5, R.raw.storm_thunder_6,
            R.raw.storm_thunder_7, R.raw.storm_thunder_8,
            R.raw.storm_thunder_9,
        ))
        load(context, THUNDER_BOOM, listOf(
            R.raw.storm_thunder_boom_1, R.raw.storm_thunder_boom_2,
        ))
    }

    private fun load(context: Context, type: String, resources: List<Int>) {
        resources.forEach { resource ->
            val sampleId = soundPool.load(context, resource, 1)
            soundTypes[sampleId] = type
        }
    }

    private fun eventType(event: String): String = when {
        event.startsWith(EXPLOSION) -> EXPLOSION
        event == THUNDER_BOOM -> THUNDER_BOOM
        event == THUNDER -> THUNDER
        else -> event
    }

    @Synchronized
    fun playEvent(event: String) {
        when (event) {
            "SAURON_START" -> {
                Log.d(TAG, "playing Eye of Sauron soundtrack")
                if (sauronPlayer.isPlaying) sauronPlayer.pause()
                sauronPlayer.setOnSeekCompleteListener { player ->
                    player.setOnSeekCompleteListener(null)
                    player.start()
                }
                sauronPlayer.seekTo(0)
                return
            }
            "SAURON_STOP" -> {
                if (sauronPlayer.isPlaying) sauronPlayer.pause()
                sauronPlayer.seekTo(0)
                return
            }
            "MATRIX_START" -> {
                Log.d(TAG, "playing Matrix soundtrack")
                if (matrixPlayer.isPlaying) matrixPlayer.pause()
                matrixPlayer.setOnSeekCompleteListener { player ->
                    player.setOnSeekCompleteListener(null)
                    player.start()
                }
                matrixPlayer.seekTo(0)
                return
            }
            "MATRIX_STOP" -> {
                if (matrixPlayer.isPlaying) matrixPlayer.pause()
                matrixPlayer.seekTo(0)
                return
            }
            "OPENING" -> {
                Log.d(TAG, "playing opening intro")
                if (openingPlayer.isPlaying) openingPlayer.pause()
                openingPlayer.setOnSeekCompleteListener { player ->
                    player.setOnSeekCompleteListener(null)
                    player.start()
                }
                openingPlayer.seekTo(0)
                return
            }
            "STORM_START" -> {
                if (!rainPlayer.isPlaying) rainPlayer.start()
                return
            }
            "STORM_STOP" -> {
                if (rainPlayer.isPlaying) rainPlayer.pause()
                rainPlayer.seekTo(0)
                return
            }
        }

        val type = eventType(event)
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
        openingPlayer.release()
        matrixPlayer.release()
        sauronPlayer.release()
        rainPlayer.release()
        soundPool.release()
    }
}
