#ifndef LEDTEST_H
#define LEDTEST_H

#include "Animation.h"

class LedTest : public Animation {
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

    const uint8_t base_brightness = 48;
    const uint8_t sweep_brightness = 120;
    const uint8_t sweep_x = ((uint16_t)(phase * 4.0f)) & 0x0F;
    const uint8_t sweep_y = ((uint16_t)(phase * 3.0f + 5.0f)) & 0x0F;
    const uint8_t sweep_z = ((uint16_t)(phase * 2.0f + 10.0f)) & 0x0F;
    const uint8_t color_phase = (uint16_t)(phase * 32.0f);

    for (uint8_t x = 0; x < Display::width; x++) {
      for (uint8_t y = 0; y < Display::height; y++) {
        for (uint8_t z = 0; z < Display::depth; z++) {
          uint8_t brightness = base_brightness;
          if (x == sweep_x || y == sweep_y || z == sweep_z) {
            brightness = sweep_brightness;
          }

          const uint8_t index = color_phase + x * 13 + y * 7 + z * 5;
          voxel(x, y, z, Color(index, RainbowGradientPalette).scale(brightness));
        }
      }
    }
  }
};
#endif
