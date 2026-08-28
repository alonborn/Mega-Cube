#ifndef WHITETEST_H
#define WHITETEST_H

#include "Animation.h"

class WhiteTest : public Animation {
 private:
  float phase;

  static float smooth(float t) { return t * t * (3.0f - 2.0f * t); }

 public:
  void init() {
    state = state_t::RUNNING;
    phase = 0;
  }

  void draw(float dt) {
    setMotionBlur(0);
    phase += dt;

    const float fade_time = 5.0f;
    const float hold_time = 3.0f;
    const float cycle = fade_time * 2.0f + hold_time;
    float t = fmodf(phase, cycle);
    float level;

    if (t < fade_time) {
      level = smooth(t / fade_time);
    } else if (t < fade_time + hold_time) {
      level = 1.0f;
    } else {
      level = 1.0f - smooth((t - fade_time - hold_time) / fade_time);
    }

    uint8_t brightness = (uint8_t)(255.0f * level);
    Color white(brightness, brightness, brightness);

    for (uint8_t x = 0; x < Display::width; x++) {
      for (uint8_t y = 0; y < Display::height; y++) {
        for (uint8_t z = 0; z < Display::depth; z++) {
          voxel(x, y, z, white);
        }
      }
    }
  }
};

#endif
