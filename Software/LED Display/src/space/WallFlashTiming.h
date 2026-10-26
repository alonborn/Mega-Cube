#ifndef WALL_FLASH_TIMING_H
#define WALL_FLASH_TIMING_H

#include <math.h>
#include <stdint.h>

namespace WallFlashTiming {
constexpr float DURATION = 0.2f;

inline bool surface(uint8_t x, uint8_t y, uint8_t z) {
  const bool xStrip = x < 2 || x >= 14;
  const bool zStrip = z < 2 || z >= 14;
  // Vertical corners are 2x1 strips on the front/back faces, not 2x2 pillars.
  const bool cornerColumn = xStrip && (z == 0 || z == 15);
  return y == 0 || cornerColumn || (y == 15 && (xStrip || zStrip));
}

class Pulse {
 private:
  float elapsed = DURATION;

 public:
  void reset() { elapsed = DURATION; }
  void trigger() { elapsed = 0; }
  void advance(float dt) { elapsed = fminf(DURATION, elapsed + fmaxf(0.0f, dt)); }
  bool active() const { return elapsed < DURATION; }
  bool lit() const { return active(); }
};
}  // namespace WallFlashTiming
#endif
