#ifndef WALL_FLASH_H
#define WALL_FLASH_H

#include "Animation.h"
#include "WallFlashTiming.h"

inline void drawWallColor(uint8_t hue) {
  const Color color(hue, RainbowGradientPalette);
  for (uint8_t x = 0; x < 16; ++x)
    for (uint8_t y = 0; y < 16; ++y)
      for (uint8_t z = 0; z < 16; ++z)
        if (WallFlashTiming::surface(x, y, z)) voxel(x, y, z, color);
}

inline void drawWallFlash(const WallFlashTiming::Pulse& pulse, uint8_t hue) {
  if (pulse.lit()) drawWallColor(hue);
}

class WallFlash : public Animation {
 private:
  WallFlashTiming::Pulse pulse;
  uint8_t hue = 0;

 public:
  void init() override {
    pulse.trigger();
    hue = random(256);
    state = state_t::RUNNING;
  }

  void draw(float dt) override {
    drawWallFlash(pulse, hue);
    pulse.advance(dt);
    if (!pulse.active()) state = state_t::INACTIVE;
  }
};
#endif
