# Live music input

Animation IDs: `ANIMATION_MUSIC_BALLS` (38), `ANIMATION_MUSIC_EQUALIZER` (39).
Both run continuously and receive input from the Android microphone via ESP32.
Existing animation IDs and the FlexIO/DMA display driver are unchanged.

The Android analyzer sends roughly 47 frames/second (48 kHz, 1024 samples).
Each BLE command is exactly 16 bytes and fits a default ATT payload.
Music uses write-without-response and high connection priority. Normal commands
use acknowledged writes, with a single in-flight GATT operation. Normal commands have priority; pending music
frames are overwritten by the latest sample, not queued.

| Byte | Meaning |
| --- | --- |
| 0, 1 | ASCII `MU` |
| 2 | Protocol version 1 |
| 3 | Frame counter, modulo 256 |
| 4 | RMS loudness, 0..255 |
| 5 | Most recent onset strength, 0..255 (retained until the next onset) |
| 6 | Onset counter, modulo 256 |
| 7..14 | Eight logarithmic frequency-band levels, 0..255 |
| 15 | XOR checksum of bytes 0..14 |

Frequency boundaries in Hz: 43, 86, 172, 345, 690, 1380, 2760, 5520, 10000.
The ESP32 validates the header/checksum and forwards bytes 3..14 as a 31-byte
UART line: `MUSIC ` + 24 uppercase hex characters + newline. The ESP32 loop is
the only UART writer, so binary telemetry cannot interleave with text commands.
No per-frame UART reply or USB log is generated.

The Teensy accepts telemetry only while one of the live animations is selected.
Input is cleared when a live animation starts. Loudness and bands use the
response curve `output = pow(input / 255, 1.8)` before smoothing: 25% input
produces about 8% output, 50% produces 29%, and 100% remains 100%.
The exponent is `LiveMusicInput::RESPONSE_EXPONENT` in `src/core/LiveMusic.h`.
Onset strength stays unchanged. Loudness and bands are smoothed
at display frame rate, with ~8 ms attack and ~120 ms decay. The current
equalizer slice is rendered on every frame at the front (Z=15); historical
slices move toward the back and are still sampled at 50 ms intervals. Onset counters
prevent the same event from firing repeatedly. After 500 ms without a new
frame, values decay to black; the equalizer history empties progressively.

After at least seven observed onsets, the Teensy fits a periodic grid to up to
16 recent events within 10 seconds (250..1600 ms periods). A candidate needs
at least six unique beat positions, 60% of the events on-grid, 70% grid
coverage, and a mean timing quality of 70% within the tolerance window.
Three successive estimates must agree on both tempo and phase before locking
(nine onsets for an ideal metronome).
This tolerates extra notes and missing beats rather than requiring
identical consecutive onset intervals. Timing tolerance is 15% or 35 ms.
Once locked, a beat clock triggers a strong blue whole-cube flash with a 45 ms
decay over either live theme. Matching onsets correct the clock; a late onset
does not flash twice for the same beat. Off-grid notes do not reset the clock.
The clock fills in missed detections for up to eight beat periods, while
telemetry is fresh and loudness exceeds 2% after the response curve.
Lost telemetry or no matching onset for eight periods clears the lock.
USB diagnostics every two seconds show received frames, onset count, lock
state, and estimated BPM during either live mode.

The analyzer detects spectral-flux onsets with a 250 ms refractory period.
This responds to percussion/transients; it is not a full musical tempo tracker.
Microphone, FFT, BLE and UART contribute latency, so visual response is not
guaranteed to be sample-aligned with playback. The existing prerecorded audio
latency setting is not applied to acoustic microphone input.

## Validation

```sh
g++ -std=c++11 -Isrc test/live_music_input.cpp -o /tmp/mega-cube-live-input-test
/tmp/mega-cube-live-input-test
/home/alon/.platformio/penv/bin/pio run
```

In Cube Android run `./gradlew testDebugUnitTest assembleDebug` to check silence,
known-frequency tones, onset refractory timing and packet/hex encoding.
