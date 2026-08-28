#ifndef GYROID_H
#define GYROID_H

#include "Animation.h"

class Gyroid : public Animation {
 private:
  float phase;
  int16_t hue;

 public:
  void init() {
    state = state_t::RUNNING;
    timer_running = 15.0f;
    phase = 0;
    hue = 0;
  }

  void draw(float dt) {
    setMotionBlur(70);
    if (timer_running.update()) {
      state = state_t::INACTIVE;
      return;
    }

    phase += dt;
    hue += dt * 92.0f;

    const float scale = 0.47f;
    const float drift = phase * 1.25f;
    const float thickness = 0.34f + 0.10f * sinf(phase * 0.7f);

    for (uint8_t x = 0; x < Display::width; x++) {
      float xf = (x - CX) * scale;
      for (uint8_t y = 0; y < Display::height; y++) {
        float yf = (y - CY) * scale;
        for (uint8_t z = 0; z < Display::depth; z++) {
          float zf = (z - CZ) * scale;

          float v = sinf(xf + drift) * cosf(yf - drift * 0.55f) +
                    sinf(yf + drift * 0.73f) * cosf(zf + drift * 0.35f) +
                    sinf(zf - drift * 0.46f) * cosf(xf + drift * 0.82f);

          float distance = fabsf(v);
          if (distance > thickness) continue;

          float edge = 1.0f - distance / thickness;
          uint8_t brightness = (uint8_t)(255.0f * edge);
          uint8_t color_index =
              (hue >> 8) + x * 17 + y * 29 + z * 41 +
              (uint8_t)(sinf(phase * 1.4f + (x + z) * 0.65f) * 40.0f);
          Color color(color_index, RainbowGradientPalette);
          voxel(x, y, z, color.scale(brightness));
        }
      }
    }
  }
};

#endif
