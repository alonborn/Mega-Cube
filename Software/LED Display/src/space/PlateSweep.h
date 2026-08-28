#ifndef PLATESWEEP_H
#define PLATESWEEP_H

#include "Animation.h"

class PlateSweep : public Animation {
 private:
  float phase;

 public:
  void init() {
    state = state_t::RUNNING;
    timer_running = 15.0f;
    phase = 0;
  }

  void draw(float dt) {
    setMotionBlur(0);
    if (timer_running.update()) {
      state = state_t::INACTIVE;
      return;
    }
    phase += dt;

    const float seconds_per_plate = 0.7f;
    const uint16_t step = phase / seconds_per_plate;
    const uint8_t axis = (step / 16) % 3;
    const uint8_t plate = step & 0x0F;

    Color color;
    if (axis == 0) {
      color = Color(120, 0, 0);
    } else if (axis == 1) {
      color = Color(0, 120, 0);
    } else {
      color = Color(0, 0, 120);
    }

    for (uint8_t x = 0; x < Display::width; x++) {
      for (uint8_t y = 0; y < Display::height; y++) {
        for (uint8_t z = 0; z < Display::depth; z++) {
          if ((axis == 0 && x == plate) ||
              (axis == 1 && y == plate) ||
              (axis == 2 && z == plate)) {
            voxel(x, y, z, color);
          }
        }
      }
    }
  }
};

#endif
