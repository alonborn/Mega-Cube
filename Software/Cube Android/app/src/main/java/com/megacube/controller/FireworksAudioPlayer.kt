package com.megacube.controller

import android.content.Context
import android.media.AudioAttributes
import android.media.MediaPlayer
import android.media.SoundPool
import android.os.Handler
import android.os.Looper
import android.os.SystemClock
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
    private val universalPlayer = MediaPlayer.create(context, R.raw.universal_intro).apply {
        isLooping = false
        setVolume(1.0f, 1.0f)
    }
    private val lightChasePlayer = MediaPlayer.create(context, R.raw.light_chase).apply {
        isLooping = false
        setVolume(1.0f, 1.0f)
    }
    private val sauronPlayer = MediaPlayer.create(context, R.raw.eye_of_sauron).apply {
        isLooping = true
        setVolume(1.0f, 1.0f)
    }
    private val rainTargetVolume = 0.22f
    private val rainFadeDurationMs = 5000L
    private val mainHandler = Handler(Looper.getMainLooper())
    private val rainPlayer = MediaPlayer.create(context, R.raw.storm_rain_loop).apply {
        isLooping = true
        setVolume(0f, 0f)
    }
    private var rainFadeStartedAt = 0L
    private val rainFade = object : Runnable {
        override fun run() {
            val progress = ((SystemClock.elapsedRealtime() - rainFadeStartedAt).toFloat() /
                rainFadeDurationMs).coerceIn(0f, 1f)
            val volume = rainTargetVolume * progress
            rainPlayer.setVolume(volume, volume)
            if (progress < 1f) mainHandler.postDelayed(this, 50L)
        }
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
            "UNIV_START" -> {
                Log.d(TAG, "playing Universal opening soundtrack")
                if (universalPlayer.isPlaying) universalPlayer.pause()
                universalPlayer.setOnSeekCompleteListener { player ->
                    player.setOnSeekCompleteListener(null)
                    player.start()
                }
                universalPlayer.seekTo(0)
                return
            }
            "UNIVERSAL_STOP" -> {
                if (universalPlayer.isPlaying) universalPlayer.pause()
                universalPlayer.seekTo(0)
                return
            }
            "LIGHT_CHASE_START" -> {
                Log.d(TAG, "playing Light Chase soundtrack")
                if (lightChasePlayer.isPlaying) lightChasePlayer.pause()
                lightChasePlayer.setOnSeekCompleteListener { player ->
                    player.setOnSeekCompleteListener(null)
                    player.start()
                }
                lightChasePlayer.seekTo(0)
                return
            }
            "LIGHT_CHASE_STOP" -> {
                if (lightChasePlayer.isPlaying) lightChasePlayer.pause()
                lightChasePlayer.seekTo(0)
                return
            }
            "STORM_START" -> {
                mainHandler.removeCallbacks(rainFade)
                rainPlayer.setVolume(0f, 0f)
                if (rainPlayer.isPlaying) rainPlayer.pause()
                rainPlayer.seekTo(0)
                rainPlayer.start()
                rainFadeStartedAt = SystemClock.elapsedRealtime()
                mainHandler.post(rainFade)
                return
            }
            "STORM_STOP" -> {
                mainHandler.removeCallbacks(rainFade)
                rainPlayer.setVolume(0f, 0f)
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
        lightChasePlayer.release()
        universalPlayer.release()
        sauronPlayer.release()
        mainHandler.removeCallbacks(rainFade)
        rainPlayer.release()
        soundPool.release()
    }
}
