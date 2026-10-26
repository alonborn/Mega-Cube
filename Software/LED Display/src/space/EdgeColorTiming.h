#ifndef EDGE_COLOR_TIMING_H
#define EDGE_COLOR_TIMING_H

#include <math.h>
#include <stdint.h>

namespace EdgeColorTiming {
constexpr float STARTS[] = {0, 0.7f, 1.3f, 1.9f, 2.5f, 2.9f, 3.2f};
constexpr uint8_t COLORS = 6;
constexpr float SECOND_STARTS[] = {0, 0.51f, 0.99f, 1.46f, 1.90f, 2.20f};
constexpr uint8_t SECOND_COLORS = 5;

class Sequence {
 private:
  const bool second;
  float elapsed;

  const float* starts() const { return second ? SECOND_STARTS : STARTS; }

 public:
  explicit Sequence(bool alternate = false) : second(alternate), elapsed(starts()[colorCount()]) {}
  uint8_t colorCount() const { return second ? SECOND_COLORS : COLORS; }
  void trigger() { elapsed = 0; }
  void advance(float dt) {
    elapsed = fminf(starts()[colorCount()], elapsed + fmaxf(0.0f, dt));
  }
  bool active() const { return elapsed < starts()[colorCount()]; }
  uint8_t index() const {
    uint8_t current = 0;
    while (current < colorCount() && elapsed >= starts()[current + 1]) ++current;
    return current;
  }
};
}  // namespace EdgeColorTiming
#endif
