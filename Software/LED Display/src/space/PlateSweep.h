#ifndef PLATESWEEP_H
#define PLATESWEEP_H

#include "Animation.h"

class PlateSweep : public Animation {
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

    const float seconds_per_plate = 0.7f;
    const uint16_t step = phase / seconds_per_plate;
    const uint8_t axis = (step / 16) % 3;
    const uint8_t plate = step & 0x0F;
    const uint8_t x_plate = (3 - (plate >> 2)) * 4 + (plate & 0x03);
    const uint8_t y_plate = 15 - plate;

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
          uint8_t z_plate = plate;
          if (axis == 2 && x >= 4 && x < 8 &&
              (plate < 4 || (plate >= 8 && plate < 12))) {
            z_plate = (plate & 0x0C) | ((plate + 2) & 0x03);
          }
          if (axis == 2 && x >= 4 && plate >= 12) {
            z_plate = (plate & 0x0C) | ((plate + 2) & 0x03);
          }

          if ((axis == 0 && x == x_plate) ||
              (axis == 1 && y == y_plate) ||
              (axis == 2 && z == z_plate)) {
            voxel(x, y, z, color);
          }
        }
      }
    }
  }
};

#endif
