#ifndef RGBTEST_H
#define RGBTEST_H

#include "Animation.h"

class RGBTest : public Animation {
 private:
  float phase;

 public:
  void init() {
    state = state_t::RUNNING;
    phase = 0;
  }

  void draw(float dt) {
    setMotionBlur(0);
    phase += dt;

    const uint8_t color_index = ((uint16_t)(phase / 2.0f)) % 3;
    Color color;
    if (color_index == 0) {
      color = Color(120, 0, 0);
    } else if (color_index == 1) {
      color = Color(0, 120, 0);
    } else {
      color = Color(0, 0, 120);
    }

    for (uint8_t x = 0; x < Display::width; x++) {
      for (uint8_t y = 0; y < Display::height; y++) {
        for (uint8_t z = 0; z < Display::depth; z++) {
          voxel(x, y, z, color);
        }
      }
    }
  }
};

#endif
