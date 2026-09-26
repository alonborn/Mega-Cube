#ifndef REDTEST_H
#define REDTEST_H

#include "Animation.h"

class RedTest : public Animation {
 private:
  static constexpr float PERIOD = 1.2f;
  float age = 0.0f;

 public:
  void init() override {
    state = state_t::RUNNING;
    age = 0.0f;
    setMotionBlur(0);
  }

  void draw(float dt) override {
    age += dt;
    const float phase = fmodf(age, PERIOD) / PERIOD * TWO_PI;
    const uint8_t level =
        static_cast<uint8_t>((0.5f - 0.5f * cosf(phase)) * 255.0f);
    const Color red = Color::RED.scaled(level);

    for (uint8_t x = 0; x < Display::width; ++x) {
      for (uint8_t y = 0; y < Display::height; ++y) {
        for (uint8_t z = 0; z < Display::depth; ++z) {
          voxel(x, y, z, red);
        }
      }
    }
  }
};

#endif
