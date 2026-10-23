# Cube Android

Native Android controller written in Kotlin and Jetpack Compose. It scans for
the Mega Cube BLE service, connects to the ESP32, and sends animation,
playlist, and brightness commands.

## Build from VS Code

After installing the Android command-line SDK and Gradle wrapper:

```bash
./gradlew assembleDebug
./gradlew installDebug
```

Enable Developer options and USB debugging on the phone before `installDebug`.

## Animation catalog

`app/src/main/java/com/megacube/controller/AnimationCatalog.kt` is the single
list for the animation picker and ESP32 synchronization. Add new entries there
with stable IDs matching the Teensy (new IDs start at 40). Names sent to the
LCD omit the ` +S` suffix and must fit 32 printable ASCII characters.

On every connection, the app queries `CATALOG?` and sends entries beyond the
ESP32's built-in defaults, one acknowledged entry at a time. The ESP32 persists
them across power cycles. No further ESP32 upload is needed when adding an
animation, unless intentionally promoting it into its built-in defaults.
The initial catalog-capable ESP32 firmware must be flashed once. Older firmware
that does not answer the query remains usable for ordinary animation commands.

## Live music

Choose `Music Balls (LIVE)` (38) or `Music Equalizer (LIVE)` (39), allow microphone
access, and keep Mega Cube in the foreground. Play music on a nearby speaker;
the source may also be a music app playing in the background on the same phone.
The microphone is stopped when the app goes to the background, disconnects,
leaves the live animation, or the Live microphone switch is turned off.
No recording is stored: only loudness, onset events, and eight frequency-band
levels are sent to the ESP32. Use Sensitivity to match the speaker volume.

Analysis uses [AudioRecord](https://developer.android.com/reference/android/media/AudioRecord)
and [JTransforms](https://github.com/wendykierp/JTransforms), 48 kHz mono,
1024-sample Hann-windowed FFT frames (~21 ms). Android 10+ uses the
low-latency VOICE_PERFORMANCE input with a VOICE_RECOGNITION fallback.
Live frames use BLE write-without-response and high connection priority;
normal commands still use acknowledged writes. This initial version uses acoustic
microphone input, not Android playback capture. Onsets respond to transients;
they are not a calibrated BPM or musical beat-position estimate.

Run `./gradlew testDebugUnitTest assembleDebug` for analysis and protocol tests.

