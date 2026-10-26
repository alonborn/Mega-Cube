#ifndef YMCA_FULL_TIMELINE_H
#define YMCA_FULL_TIMELINE_H

#include <stdint.h>
#include "YMCAPose.h"

namespace YMCAFullTimeline {
// Positive offset advances the visuals relative to the soundtrack.
constexpr float VISUAL_OFFSET_SECONDS = 0.0f;
// Delay only lighting effects; leave the dancer and soundtrack timing unchanged.
constexpr float LIGHTING_DELAY_SECONDS = 0.15f;
constexpr float WALL_HOLD_EXTRA_DELAY_SECONDS = 0.0f;
constexpr float TRACK_SECONDS = 221.034f;
constexpr float END_FADE_SECONDS = 10.0f;
constexpr float LETTER_SECONDS = YMCAPose::LETTER_DURATION;
constexpr float DANCE_TRANSITION_SECONDS = 0.25f;
constexpr float DANCE_DURATION_SCALE = 2.0f;

inline float lightingTime(float showSeconds, uint8_t animation = 0) {
  return showSeconds - LIGHTING_DELAY_SECONDS -
      (animation == 46 ? WALL_HOLD_EXTRA_DELAY_SECONDS : 0.0f);
}

inline float endFade(float songSeconds) {
  return YMCAPose::smooth((TRACK_SECONDS - songSeconds) / END_FADE_SECONDS);
}

struct Cue { float seconds; uint8_t animation; };

// Absolute song times in seconds. Edit the first column to fine-tune each cue.
constexpr Cue CUES[] = {
    {  0.00f, 48}, //  0  00:00.00
    { 11.68f, 46}, //  1  00:11.68
    { 15.44f, 46}, //  2  00:15.44
    { 19.27f, 46}, //  3  00:19.27
    { 23.01f, 47}, //  4  00:23.01
    { 26.77f, 46}, //  5  00:26.77
    { 30.55f, 46}, //  6  00:30.55
    { 34.35f, 46}, //  7  00:34.35
    { 38.15f, 47}, //  8  00:38.15
    { 41.82f, 49}, //  9  00:41.82
    { 45.53f, 40}, // 10  00:45.53
    { 60.61f, 40}, // 11  01:00.61
    { 75.88f, 46}, // 12  01:15.88
    { 79.39f, 46}, // 13  01:19.39
    { 83.17f, 46}, // 14  01:23.17
    { 86.93f, 47}, // 15  01:26.93
    { 90.67f, 46}, // 16  01:30.67
    { 94.51f, 46}, // 17  01:34.51
    { 98.34f, 46}, // 18  01:38.34
    {102.17f, 47}, // 19  01:42.17
    {105.81f, 49}, // 20  01:45.81
    {109.52f, 40}, // 21  01:49.52
    {117.14f, 46}, // 22  01:57.14
    {118.98f, 46}, // 23  01:58.98
    {120.89f, 46}, // 24  02:00.89
    {124.64f, 40}, // 25  02:04.64
    {132.23f, 46}, // 26  02:12.23
    {134.10f, 46}, // 27  02:14.10
    {135.91f, 46}, // 28  02:15.91
    {139.72f, 46}, // 29  02:19.72
    {143.51f, 46}, // 30  02:23.51
    {147.30f, 46}, // 31  02:27.30
    {151.05f, 47}, // 32  02:31.05
    {154.79f, 46}, // 33  02:34.79
    {158.48f, 46}, // 34  02:38.48
    {162.34f, 46}, // 35  02:42.34
    {166.14f, 47}, // 36  02:46.14
    {169.82f, 49}, // 37  02:49.82
    {173.55f, 40}, // 38  02:53.55
    {182.97f, 46}, // 39  03:02.97
    {184.93f, 46}, // 40  03:04.93
    {186.79f, 46}, // 41  03:06.79
    {188.69f, 40}, // 42  03:08.69
    {196.21f, 46}, // 43  03:16.21
    {198.16f, 46}, // 44  03:18.16
    {200.00f, 46}, // 45  03:20.00
    {201.86f, 46}, // 46  03:21.86
    {203.50f, 40}, // 47  03:23.50
};
constexpr uint8_t CUE_COUNT = sizeof(CUES) / sizeof(CUES[0]);

class Cursor {
 private:
  uint8_t next = 0;

 public:
  void reset() { next = 0; }
  int nextDue(float seconds) {
    if (next < CUE_COUNT && seconds >= CUES[next].seconds) return next++;
    return -1;
  }
};
}  // namespace YMCAFullTimeline
#endif
