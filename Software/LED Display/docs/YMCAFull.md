# YMCA Full (50)

Select **YMCA Full +S** in the phone app. The phone plays the soundtrack from
`Software/ymca/ymca.mkv` (audio remuxed without re-encoding into
`Cube Android/app/src/main/res/raw/ymca_full.m4a`). Its duration is 221.034 seconds.
The Teensy sends `YMCA_START` / `YMCA_STOP` using the existing ESP event relay.
No ESP firmware update is required.

## Editing the show

Edit `src/space/YMCAFullTimeline.h`:

- `CUES`: each entry is `{absolute_seconds, animation_id}`. Times are measured
  from the beginning of the music, not from the preceding entry. Keep entries
  strictly increasing. For example, `01:15.88` is `75.88f`.
- `VISUAL_OFFSET_SECONDS`: positive values move the whole visual show earlier;
  negative values delay it. Existing phone audio-route latency compensation
  still applies separately.
- Phone route compensation is 350 ms for Bluetooth A2DP and 280 ms for BLE
  Audio (100 ms more than the original settings), and zero for phone/non-BT
  outputs. Connect the speaker and restart the show to apply the route's
  start-time compensation; this is separate from the 0.15-second lighting offset.
- `TRACK_SECONDS`: track end; the show remains black and silent after this time.
- `END_FADE_SECONDS`: smooth whole-scene LED fade over the last ten seconds
  (211.034..221.034). The soundtrack and cue timings are unchanged. This scales
  only the rendered frame, not the user's master brightness or future animations.
- `LIGHTING_DELAY_SECONDS`: delay for all lighting effects and letter color
  holds only, currently 0.15 seconds; the cue table and dancer are unchanged.
- `WALL_HOLD_EXTRA_DELAY_SECONDS`: optional extra delay only for ID46 instances
  within Full, currently zero (net delay 0.15 seconds, 0.1 seconds earlier than
  the prior version). ID40,
  ID47, ID49, standalone ID46 and phone Bluetooth compensation are unchanged.
- `LETTER_SECONDS`: shared from `YMCAPose::LETTER_DURATION`; each ID40 passage
  lasts two independently timed Y-M-C-A cycles (7.51 seconds). Edit
  `LETTER_STARTS` in `YMCAPose.h` for individual arrivals, shared with ID40.
  Arrivals are 0/.92/1.32/1.62/3.74/4.67/5.13/5.39 seconds. The final A holds
  for 2.12 seconds, like the first A, before resuming random dance.
- `DANCE_TRANSITION_SECONDS`: blend duration between random dance modes.
- `DANCE_DURATION_SCALE`: random dance hold multiplier, currently 2.0; also
  scales the first dance after stage construction, not scheduled letter cues.

Only the Teensy needs reflashing after editing these cues. Phone rebuilding is
needed only if changing its embedded music or adding another catalog entry.

## Layers

The initial ID48 builds the stage and raises the figure. ID46 holds a random
edge/floor color for 0.2 seconds; ID47 and ID49 use their existing color holds.
All use the shared 2x1 edge/full-floor mask. Lighting never resets the dancer.
ID40 temporarily overrides the dancer with two letter cycles and changes the
stage color at each letter arrival plus the lighting delay. Each color stays
continuously until the next letter's color, without flashes. Before the second
Y, a shared 1.62-second rainbow break reveals the underlying disco colors,
starting after the first A has held its uniform color for 0.5 seconds;
this lighting-only interval also respects `LIGHTING_DELAY_SECONDS`. The last
A's color stays until the delayed letter-lighting window ends. Other passages choose different Side Steps,
March, Disco, Clap, Perimeter and Hip Sway modes every 7.2..12.6 seconds (twice the original
hold duration); scheduled letter passages still interrupt on time. Random March
travel and worms continue independently of lighting cues. Choreography and
letter gestures use their shared measured timing; walking/disco movement
cycles are 0.9 seconds and Clap uses a separate 0.94-second period in both
the standalone animation and Full.

The final letter passage starts at `03:23.50` (203.5 seconds).
During each letters passage, the corresponding 13x13 rainbow Y/M/C/A glyph
appears only on the rear face (Z=15, confirmed on the physical cube), without
horizontal mirroring for viewing from the front (Z=0). The other sides, ceiling and floor receive no glyphs.
Plain block letters retain thick main strokes and share the uniform frame
color's timing, including its 0.15-second lighting delay. The first A projection
lasts only 0.5 seconds and disappears for the rainbow break; body poses are
unchanged.
This first version does not restart at the end. Selecting another animation
stops the soundtrack; temporary effects 40/46/47/49 preserve it. Selecting
YMCA Full again restarts both the music and show from zero.

The show uses absolute elapsed microseconds, not accumulated animation frame
times, so it does not accumulate drift. Events crossed during a delayed frame
are processed in order; each layer is sampled at its actual song-relative age.

## Tests

```bash
g++ -std=c++11 -Isrc test/ymca_full_timeline.cpp -o /tmp/ymca-full-test
/tmp/ymca-full-test
```
