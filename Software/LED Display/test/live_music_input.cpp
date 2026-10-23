#include "core/LiveMusic.h"
#include <assert.h>
#include <stdio.h>

static void frame(LiveMusicInput& input, uint8_t sequence, uint8_t beat, uint32_t now) {
  char hex[25];
  snprintf(hex, sizeof(hex), "%02XFFFF%02XFFFFFFFFFFFFFFFF",
           static_cast<unsigned>(sequence), static_cast<unsigned>(beat));
  assert(input.accept(hex, now));
}

int main() {
  LiveMusicInput input;
  assert(!input.accept("00", 0));
  assert(!input.accept("00FF0000000000000000000G", 0));
  assert(input.accept("00FF000000FF000000000000", 100));
  input.advance(0.02f, 101);
  assert(input.level > 0.9f && input.bands[1] > 0.9f);
  input.advance(0.1f, 101);
  assert(input.level > 0.9f && input.bands[1] > 0.9f);
  assert(input.beat == 0.0f);
  assert(!input.accept("00FF000000FF000000000000", 102));
  assert(input.accept("01FFFF0100FF000000000000", 150));
  assert(input.beat == 1.0f);
  assert(input.accept("02FFFF0100FF000000000000", 200));
  input.advance(0.1f, 201);
  assert(input.beat < 0.6f);
  for (int i = 0; i < 30; ++i) input.advance(0.1f, 1000 + i * 100);
  assert(input.level < 0.001f && input.bands[1] < 0.001f);
  input.reset();
  assert(input.level == 0.0f && input.beat == 0.0f);
  assert(input.accept("FFFF00000000000000000000", 0xffffff00u));
  input.advance(0.1f, 10u);
  assert(input.level > 0.9f);
  input.reset();
  assert(input.accept("008000008080808080808080", 0));
  input.advance(0.02f, 1);
  assert(input.level > 0.25f && input.level < 0.30f);
  input.advance(0.1f, 2);
  assert(input.level > 0.28f && input.level < 0.30f);
  assert(fabsf(input.bands[0] - input.level) < 0.001f);
  input.reset();
  assert(input.accept("004000004040404040404040", 0));
  input.advance(0.1f, 1);
  assert(input.level > 0.07f && input.level < 0.10f);
  assert(fabsf(input.bands[7] - input.level) < 0.001f);
  input.reset();
  frame(input, 0, 0, 0);
  for (uint8_t i = 1; i <= 9; ++i) {
    frame(input, i, i, i * 500);
    assert(input.rhythmStable == (i == 9));
  }
  assert(input.rhythmPulse == 1.0f);
  input.advance(0.05f, 4510);
  assert(input.rhythmPulse > 0.3f && input.rhythmPulse < 0.4f);
  frame(input, 10, 9, 4520);
  assert(input.rhythmPulse < 0.4f);
  frame(input, 11, 10, 4800);
  assert(input.rhythmStable && input.rhythmPulse < 0.4f);
  frame(input, 12, 12, 5500);
  assert(input.rhythmStable);
  assert(input.rhythmPulse == 1.0f);
  assert(input.tempoBpm() > 118.0f && input.tempoBpm() < 122.0f);
  // Keep flashing on the measured grid even when the analyzer misses a beat.
  frame(input, 13, 12, 5980);
  input.advance(0.1f, 5980);
  assert(input.rhythmPulse < 0.2f);
  frame(input, 14, 12, 6000);
  input.advance(0.02f, 6000);
  assert(input.rhythmStable && input.rhythmPulse > 0.95f);
  // A slightly late detection must not create a second full flash.
  frame(input, 15, 13, 6030);
  input.advance(0.03f, 6030);
  assert(input.rhythmPulse < 0.6f);
  frame(input, 16, 13, 10200);
  input.advance(0.01f, 10200);
  assert(!input.rhythmStable && input.rhythmPulse == 0.0f);
  assert(input.level > 0.7f);
  input.reset();
  frame(input, 0, 0, 0);
  const uint32_t syncopated[] = {500, 1000, 1250, 1500, 2000, 3000, 3500, 4000, 4500, 5000, 5500};
  for (uint8_t i = 0; i < 11; ++i) frame(input, i + 1, i + 1, syncopated[i]);
  assert(input.rhythmStable);
  assert(input.tempoBpm() > 115.0f && input.tempoBpm() < 125.0f);
  assert(input.accept("1400000B0000000000000000", 6000));
  input.advance(0.1f, 6000);
  assert(input.rhythmPulse < 0.12f);
  assert(input.accept("1500000B0000000000000000", 6500));
  input.advance(0.1f, 6500);
  assert(input.rhythmPulse < 0.02f);
  input.reset();
  frame(input, 0, 0, 0);
  const uint32_t irregular[] = {350, 810, 1520, 1870, 2710, 3260, 4100, 4810, 5400, 6180, 6710, 7390};
  for (uint8_t i = 0; i < 12; ++i) frame(input, i + 1, i + 1, irregular[i]);
  assert(!input.rhythmStable);
  input.reset();
  frame(input, 0, 250, 0);
  for (uint8_t i = 1; i <= 9; ++i) frame(input, i, static_cast<uint8_t>(250 + i), i * 1000);
  assert(input.rhythmStable);
  input.advance(0.01f, 9600);
  assert(!input.rhythmStable && input.rhythmPulse == 0.0f);
  puts("Live music: beat grid, extra/missed onsets, irregular input, pulse and timeout OK");
}
