#include "space/WallFlashTiming.h"
#include "space/YMCAPose.h"
#include <assert.h>
#include <stdio.h>

int main() {
  using namespace WallFlashTiming;
  Pulse pulse;
  assert(!pulse.active() && !pulse.lit());
  pulse.trigger();
  assert(pulse.active() && pulse.lit());
  for (int frame = 0; frame < 200; ++frame) {
    pulse.trigger();
    pulse.advance(frame * 0.001f);
    assert(pulse.active() && pulse.lit());
  }
  pulse.trigger();
  pulse.advance(DURATION);
  assert(!pulse.active() && !pulse.lit());
  pulse.advance(1);
  assert(!pulse.active() && !pulse.lit());
  pulse.trigger();
  pulse.advance(-1);
  assert(pulse.lit());
  pulse.reset();
  assert(!pulse.active() && !pulse.lit());
  int count = 0;
  for (uint8_t x = 0; x < 16; ++x)
    for (uint8_t y = 0; y < 16; ++y)
      for (uint8_t z = 0; z < 16; ++z) {
        const bool boundary = surface(x, y, z);
        const bool floor = y == 0;
        const bool cornerColumn = (x <= 1 || x >= 14) && (z == 0 || z == 15);
        const bool topRim = y == 15 && (x <= 1 || x >= 14 || z <= 1 || z >= 14);
        assert(boundary == (floor || cornerColumn || topRim));
        if (boundary) ++count;
      }
  assert(count == 480);
  assert(surface(1, 8, 0) && surface(14, 8, 15));
  assert(!surface(1, 8, 1) && !surface(14, 8, 14));
  assert(!surface(2, 8, 0) && !surface(13, 8, 15));
  assert(surface(1, 15, 8) && surface(8, 15, 14));
  assert(!surface(2, 15, 8) && !surface(8, 15, 13));
  assert(surface(0, 15, 8) && surface(15, 15, 8));
  assert(surface(8, 15, 0) && surface(8, 15, 15));
  assert(!surface(8, 15, 8) && !surface(0, 8, 8) && !surface(8, 8, 0));
  const float cues[] = {0, 0.92f, 1.32f, 1.62f, 3.74f, 4.67f, 5.13f, 5.39f};
  for (int index = 0; index < 8; ++index) {
    assert(YMCAPose::letterIndex(cues[index]) == index);
    if (index) assert(YMCAPose::letterIndex(cues[index] - 0.0001f) == index - 1);
  }
  int changes = 0, last = -1;
  for (int frame = 0; frame < 7510; ++frame) {
    const int current = YMCAPose::letterIndex(frame * 0.001f);
    if (current != last) ++changes;
    last = current;
  }
  assert(changes == 8);
  assert(YMCAPose::letterIndex(7.51f) == 0);
  puts("Wall color hold: continuous 200ms, 480 floor/2x1-edge LEDs, eight YMCA cues OK");
}
