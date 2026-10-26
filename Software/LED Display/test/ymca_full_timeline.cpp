#include "space/YMCAFullTimeline.h"
#include "space/YMCAPose.h"
#include "space/EdgeColorTiming.h"
#include "space/StageBuildTiming.h"
#include <assert.h>
#include <stdio.h>

int main() {
  using namespace YMCAFullTimeline;
  assert(CUE_COUNT == 48);
  assert(END_FADE_SECONDS == 10);
  const float fadeStart = TRACK_SECONDS - 10;
  assert(endFade(0) == 1 && endFade(fadeStart) == 1);
  assert(fabsf(endFade(TRACK_SECONDS - 5.0f) - 0.5f) < 0.0001f);
  assert(endFade(TRACK_SECONDS) == 0 && endFade(TRACK_SECONDS + 10) == 0);
  float previousBrightness = 1;
  for (int frame = 0; frame <= 1000; ++frame) {
    const float level = endFade(fadeStart + frame * 0.01f);
    assert(level >= 0 && level <= 1 && level <= previousBrightness);
    previousBrightness = level;
  }
  assert(DANCE_DURATION_SCALE == 2.0f);
  assert(fabsf(4 * YMCAPose::MOVEMENT_SECONDS * DANCE_DURATION_SCALE - 7.2f) < 0.0001f);
  assert(fabsf(7 * YMCAPose::MOVEMENT_SECONDS * DANCE_DURATION_SCALE - 12.6f) < 0.0001f);
  assert(fabsf(LIGHTING_DELAY_SECONDS - 0.15f) < 0.0001f);
  assert(WALL_HOLD_EXTRA_DELAY_SECONDS == 0.0f);
  assert(CUES[0].seconds == 0 && CUES[0].animation == 48);
  assert(StageBuildTiming::READY <= CUES[1].seconds);
  int letters = 0, hold = 0, first = 0, second = 0;
  for (int index = 0; index < CUE_COUNT; ++index) {
    const auto cue = CUES[index];
    if (index) assert(cue.seconds > CUES[index - 1].seconds);
    assert(cue.seconds < TRACK_SECONDS);
    assert(lightingTime(cue.seconds + 0.149f) < cue.seconds);
    assert(fabsf(lightingTime(cue.seconds + 0.15f) - cue.seconds) < 0.0001f);
    const float extraDelay = 0.0f;
    const float onset = cue.seconds + LIGHTING_DELAY_SECONDS + extraDelay;
    assert(lightingTime(onset - 0.001f, cue.animation) < cue.seconds);
    assert(fabsf(lightingTime(onset, cue.animation) - cue.seconds) < 0.0001f);
    assert(fabsf(lightingTime(cue.seconds, cue.animation) - lightingTime(cue.seconds) + extraDelay) < 0.0001f);
    if (cue.animation == 46) {
      const float previousOnset = cue.seconds + LIGHTING_DELAY_SECONDS + 0.1f;
      assert(fabsf(previousOnset - onset - 0.1f) < 0.0001f);
    }
    if (cue.animation == 40) ++letters;
    else if (cue.animation == 46) ++hold;
    else if (cue.animation == 47) ++first;
    else if (cue.animation == 49) ++second;
    else assert(index == 0 && cue.animation == 48);
    Cursor cursor;
    for (int previous = 0; previous < index; ++previous)
      assert(cursor.nextDue(cue.seconds) == previous);
    if (index) assert(cursor.nextDue(cue.seconds - 0.0001f) == -1);
    assert(cursor.nextDue(cue.seconds) == index);
    assert(cursor.nextDue(cue.seconds) == -1);
  }
  assert(letters == 7 && first == 6 && second == 3 && hold == 31);
  assert(CUES[47].seconds == 203.5f && CUES[47].animation == 40);
  assert(CUES[47].seconds + LETTER_SECONDS < TRACK_SECONDS);
  assert(LETTER_SECONDS == YMCAPose::loopSeconds(YMCAPose::Mode::LETTERS));
  assert(fabsf(LETTER_SECONDS - 7.51f) < 0.0001f);
  for (int letter = 0; letter < YMCAPose::LETTER_COUNT; ++letter) {
    const float time = CUES[10].seconds + YMCAPose::LETTER_STARTS[letter];
    assert(lightingTime(time + 0.149f) < time);
    assert(fabsf(lightingTime(time + 0.15f) - time) < 0.0001f);
    const float base = CUES[10].seconds;
    const float start = base + YMCAPose::LETTER_STARTS[letter] + LIGHTING_DELAY_SECONDS;
    const float end = base + YMCAPose::LETTER_STARTS[letter + 1] + LIGHTING_DELAY_SECONDS;
    assert(YMCAPose::letterColorIndex(lightingTime(start + 0.0001f) - base) == letter);
    assert(YMCAPose::letterColorIndex(lightingTime(end - 0.0001f) - base) == (letter == 3 ? -1 : letter));
  }
  const float gapStart = CUES[10].seconds + YMCAPose::LETTER_COLOR_BREAK_START + LIGHTING_DELAY_SECONDS;
  const float gapEnd = CUES[10].seconds + YMCAPose::LETTER_STARTS[4] + LIGHTING_DELAY_SECONDS;
  assert(fabsf(gapEnd - gapStart - 1.62f) < 0.0001f);
  const float firstAColor = CUES[10].seconds + YMCAPose::LETTER_STARTS[3] + LIGHTING_DELAY_SECONDS;
  assert(fabsf(gapStart - firstAColor - 0.5f) < 0.0001f);
  // The projected letter and uniform frame share this exact lighting window.
  assert(YMCAPose::letterColorIndex(lightingTime(firstAColor + 0.499f) - CUES[10].seconds) == 3);
  assert(YMCAPose::letterColorIndex(lightingTime(firstAColor + 0.501f) - CUES[10].seconds) == -1);
  assert(YMCAPose::letterColorIndex(lightingTime(gapStart + 0.0001f) - CUES[10].seconds) == -1);
  assert(YMCAPose::letterColorIndex(lightingTime(gapEnd - 0.0001f) - CUES[10].seconds) == -1);
  assert(YMCAPose::letterColorIndex(lightingTime(gapEnd + 0.0001f) - CUES[10].seconds) == 4);
  // Dropped frames and different render rates must deliver every absolute cue once.
  const float steps[] = {0.004f, 0.01f, 0.017f, 0.05f, 0.8f};
  for (float step : steps) {
    Cursor cursor;
    int expected = 0;
    for (float time = -0.5f; time < TRACK_SECONDS + step; time += step) {
      int index;
      while ((index = cursor.nextDue(time)) >= 0) assert(index == expected++);
    }
    assert(expected == CUE_COUNT);
    assert(cursor.nextDue(1000) == -1);
    cursor.reset();
    assert(cursor.nextDue(-0.001f) == -1 && cursor.nextDue(0) == 0);
  }
  puts("YMCA Full: all 48 absolute cues, layer IDs, seven double-letter passages, final 03:23.50, no skipped/repeated cues OK");
}
