# YMCA choreography studies

Six independent looping animations, with no audio attached yet:

| ID | Animation | Movement |
| --- | --- | --- |
| 40 | YMCA Letters | Two measured cycles: 0/.92/1.32/1.62, then 3.74/4.67/5.13/5.39 seconds |
| 41 | YMCA Side Steps | Alternating side steps, arm waves and a gentle bounce |
| 42 | YMCA March | Raised knees, counter-moving arms, slow random travel within X/Z +/-3.8 LEDs |
| 43 | YMCA Disco | Alternating high diagonal points, low opposite hand and hip sway |
| 44 | YMCA Clap | One overhead clap every 0.94 seconds |
| 45 | YMCA Perimeter | Gentle walking around a rounded-square perimeter, turning along the path |
| 46 | Wall Color Hold | One continuous 200ms floor/corner/top-rim color hold; previous animation continues |
| 47 | Edge Color Sequence | Six held random colors on all edges and floor, then resume |
| 48 | Stage Build | Sequential edge beams, diagonal floor wash, dancer rising into a pulsing stage |
| 49 | Edge Color Sequence 2 | Alternate five-color edge/floor sequence, then resume |
| 50 | YMCA Full +S | Complete soundtrack show with an editable absolute cue table |
| 51 | YMCA Hip Sway | Side-to-side pelvis sway, hands resting on hips and elbows out |

The figure is approximately 14 LED rows tall, matching the previous taller
figure. Bright cyan arms/white hands carry the choreography; the narrow body
and blue legs are dimmer to keep the gestures distinct. The head is a sparse
rounded outline with a little depth. There is no ground pattern or state machine.
All six modes have a colorful disco frame: lower/upper square outlines and
four vertical edges, with moving rainbow colors and gentle brightness pulses.
Colors now advance four times faster than the previous version (twelve times
the first frame version). Vertical gradients travel downwards at 32 LEDs/second.
Diagonal color waves move across
the 4x4 tile floor rather than just changing each tile's color in place.
The 12 edges (176 unique LEDs) surround the colored tile floor at Y=0.
Floor brightness stays below the figure, with dim tile boundaries; the figure
is drawn last so the feet remain clear. The top face remains empty and side-face
effects stay sparse for visibility.
Short horizontal rainbow trails also run along the four side faces, from one
corner column to the adjacent one. Heights (Y=2..13), colors, directions,
lengths (3..6 LEDs) and speeds (20..32 LEDs/second) are randomized. New trails
spawn every 0.14..0.3 seconds with a dimming gradient behind each head. A second,
independent group runs vertically up/down at random horizontal positions across
all four side faces (including the corners), at the same spawn frequency.
At most eight trails per orientation are stored, and none cross
the cube interior. Their state continues across
choreography loops and resets with a new seed when an animation is selected.

Movement cycles follow the measured 0.9-second pulse (`MOVEMENT_SECONDS` in
`src/space/YMCAPose.h`): one full left/right cycle per pulse, with
individual alternating steps every 0.45 seconds. Disco changes side every 0.9
seconds. Letters arrive at the measured cues via 0.08-second transitions;
the two letter cycles have independent measured intervals, not equal durations.
Letters loop after 7.51 seconds, March/Disco/Side Steps after 7.2 seconds,
and Clap after 7.52 seconds (eight 0.94-second claps),
and Perimeter after 14.4 seconds. The perimeter route has a 4.6-LED radius and rounded corners,
with enough clearance for the complete figure and its head outline.
March travel is independent of the looping gait: random destinations, smooth
turns and eased motion capped at 1.35 LEDs/second. Each activation gets a new
random seed; travel does not reset at the gait loop boundary. The feet retain
their 0.9-second left/right cycle regardless of travel speed or direction.
The frame and floor have an independent seamless 2-second color loop,
so faster choreography does not restart its colors. Poses are independent
of rendering, so a later music timeline can choose modes and drive their time.

`YMCA.h` renders the shared figure; `YMCAPose.h` defines the choreography.
Names synchronize from the phone using the existing ESP catalog protocol.
Reused IDs 40..45 replace the previous figure names through normal catalog sync.

Selecting ID40 over UART runs exactly two letter cycles (7.51 seconds), then
reveals the previously selected animation without reinitializing it. That
animation and its audio continue underneath; the phone sends this as a temporary
effect and the Teensy replies `OK EFFECT 40`. Selecting a regular animation or
another temporary effect cancels the letter overlay; selecting ID40 again
restarts its two cycles. Other YMCA modes remain continuous loops.

Letters also trigger the color hold on the floor, four corner columns and top rim at Y/M/C/A arrival
cues (0, .92, 1.32, 1.62, 3.74, 4.67, 5.13, 5.39 seconds). Each letter gets a new random hue, held
continuously until the next letter; the last A holds until the letter passage
ends. Between the first A and second Y, the uniform-color overlay is removed
for 1.62 seconds (2.12..3.74) to reveal the moving rainbow frame/floor. The
first A keeps its uniform color for exactly 0.5 seconds. Letter poses and
arrival times do not change. `FIRST_A_COLOR_SECONDS` controls the hold, shared
with Full (whose lighting delay still applies).
The shared temporary-effect mask uses 2x1 vertical
corner strips on the front/back faces and a two-LED-wide rim on the top plane,
plus the full floor (480 LEDs total). It does not use 2x2 corner pillars.
The side-face centers, ceiling center and interior stay
untouched to keep the dancer visible. ID46 is a temporary overlay when
selected over UART: it does not change the selected animation, playlist, or
soundtrack and replies `OK EFFECT 46` rather than an animation-selection ACK.
It expires after 0.2 seconds, without reinitializing the underlying animation.
No ESP firmware update is needed; the phone syncs its new catalog entry.

Letters mode additionally projects the matching glyph onto the rear face
(Z=15, confirmed on the physical cube) only, without horizontal mirroring for the front (Z=0)
through the cube. No other faces receive glyphs. Compact 13x13 block letters
retain three-LED main strokes, with narrower diagonal/gap cells and clear
openings. This replaces the less readable Scroller font used in the first
version. Only glyph pixels
are added, with a moving rainbow gradient; empty pixels keep the dancer visible.
The glyph uses exactly the uniform edge/floor color window. In particular,
the first projected A lasts 0.5 seconds, then disappears during the rainbow
break; the second Y restores the projection. Full's lighting delay applies to
the projected glyph too, while the dancer's gesture timings stay unchanged.

ID47 is another temporary overlay. Colors start immediately and change at
0.7, 1.3, 1.9, 2.5 and 2.9 seconds; it ends at 3.2 seconds. Thus the six colors
last 0.7/0.6/0.6/0.6/0.4/0.3 seconds. Each color differs from its predecessor.
It uses the same floor/12-edge mask, preserves animation and soundtrack state,
and replies `OK EFFECT 47`. Selecting either effect replaces the other;
selecting a regular animation cancels the current effect.

ID49 uses the same renderer with independent cues at 0, 0.51, 0.99, 1.46 and
1.90 seconds. The final color is held for 0.3 seconds, matching ID47, so it
finishes at 2.20 seconds. The original ID47 timings are unchanged. It replies
`OK EFFECT 49` and preserves the underlying animation and soundtrack.

Stage Build (48) starts black for about 0.36 seconds. Each of the 12 edges builds
over about 0.51 seconds, with a white leading tip and a retained colored trail.
Edges use 2x1 thickness. A 1.71-second diagonal floor wash follows, then a
0.57-second anticipation and a 2.86-second rise from below the cube. All opening
phases are scaled together to add 3.5 seconds to the original 8.17 seconds. The figure
uses exactly the shared YMCA renderer and arrives centered on the floor.
After about 11.67 seconds, the fully built stage and figure stay visible with
cycling colors and gentle pulses until another animation is selected. It does
not restart its construction automatically and has no audio attached yet.

```bash
g++ -std=c++11 -Isrc test/ymca_pose.cpp -o /tmp/ymca-pose-test
/tmp/ymca-pose-test
```
