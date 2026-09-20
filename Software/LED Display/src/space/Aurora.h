#ifndef AURORA_H
#define AURORA_H

#include "Animation.h"

class Aurora : public Animation {
 private:
  static constexpr float DURATION = 30.0f;
  float age = 0.0f;

  static uint8_t intensity(float distance, float width, float shimmer) {
    if (distance >= width) return 0;
    float value = (1.0f - distance / width) * shimmer;
    value *= value;
    return static_cast<uint8_t>(255.0f * min(1.0f, value));
  }

 public:
  void init() override {
    state = state_t::RUNNING;
    age = 0.0f;
    setMotionBlur(205);
  }

  void draw(float dt) override {
    age += dt;
    const float phase = age * TWO_PI / DURATION;

    for (uint8_t x = 0; x < Display::width; ++x) {
      for (uint8_t z = 0; z < Display::depth; ++z) {
        const float xf = x - CX;
        const float zf = z - CZ;

        const float center_green =
            CY + 4.6f * sinf(xf * 0.47f + 5.0f * phase +
                            0.8f * sinf(zf * 0.31f - 2.0f * phase)) +
            1.6f * sinf(zf * 0.58f + 3.0f * phase);
        const float center_blue =
            CY + 4.15f * sinf(xf * 0.39f - 4.0f * phase + 2.1f) +
            1.95f * sinf(zf * 0.36f + 3.0f * phase + 0.7f);
        const float shimmer_green =
            0.58f +
            0.42f * (0.5f + 0.5f * sinf(zf * 0.78f - 10.0f * phase));
        const float shimmer_blue =
            0.50f +
            0.38f * (0.5f + 0.5f * sinf(xf * 0.64f + 8.0f * phase));

        for (uint8_t y = 0; y < Display::height; ++y) {
          Color color = Color::BLACK;

          const uint8_t green =
              intensity(fabsf(y - center_green), 1.05f, shimmer_green);
          const uint8_t blue =
              intensity(fabsf(y - center_blue), 0.90f, shimmer_blue);
          color.maximize(Color(15, 255, 35).scaled(green));
          color.maximize(Color(25, 45, 255).scaled(blue));

          if (!color.isBlack()) voxel(x, y, z, color);
        }
      }
    }

    if (age >= DURATION) state = state_t::INACTIVE;
  }
};

#endif
