#include "space/EdgeColorTiming.h"
#include <assert.h>
#include <stdio.h>

int main() {
  using namespace EdgeColorTiming;
  assert(COLORS == 6);
  assert(fabsf(STARTS[COLORS] - STARTS[COLORS - 1] - 0.3f) < 0.0001f);
  Sequence sequence;
  assert(!sequence.active() && sequence.index() == COLORS);
  for (int index = 0; index < COLORS; ++index) {
    sequence.trigger();
    sequence.advance(STARTS[index]);
    assert(sequence.active() && sequence.index() == index);
    sequence.trigger();
    sequence.advance(STARTS[index + 1] - 0.0001f);
    assert(sequence.active() && sequence.index() == index);
  }
  sequence.trigger();
  assert(sequence.index() == 0 && sequence.active());
  sequence.advance(-1);
  assert(sequence.index() == 0);
  sequence.advance(STARTS[COLORS]);
  assert(!sequence.active() && sequence.index() == COLORS);
  sequence.trigger();
  sequence.advance(100);
  assert(!sequence.active() && sequence.index() == COLORS);
  sequence.trigger();
  for (int frame = 0; frame < 330; ++frame) sequence.advance(0.01f);
  assert(!sequence.active());
  puts("Edge colors: six colors, 0.7/0.6/0.6/0.6/0.4/0.3s holds, 3.2s completion and retrigger OK");
  Sequence second(true);
  assert(!second.active() && second.colorCount() == 5);
  const float durations[] = {0.51f, 0.48f, 0.47f, 0.44f, 0.3f};
  for (int index = 0; index < SECOND_COLORS; ++index) {
    assert(fabsf(SECOND_STARTS[index + 1] - SECOND_STARTS[index] - durations[index]) < 0.0001f);
    second.trigger();
    second.advance(SECOND_STARTS[index]);
    assert(second.active() && second.index() == index);
    second.trigger();
    second.advance(SECOND_STARTS[index + 1] - 0.0001f);
    assert(second.active() && second.index() == index);
  }
  second.trigger();
  second.advance(2.20f);
  assert(!second.active() && second.index() == SECOND_COLORS);
  second.trigger();
  assert(second.active() && second.index() == 0);
  puts("Edge colors 2: five colors, 0.51/0.48/0.47/0.44/0.3s holds, 2.2s completion OK");
}
