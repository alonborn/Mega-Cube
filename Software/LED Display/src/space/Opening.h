#ifndef OPENING_H
#define OPENING_H

#include "Animation.h"

float getAudioLeadSeconds();

class Opening : public Animation {
 private:
  static constexpr float GROW_DURATION = 2.15f;
  static constexpr float HOLD_DURATION = 0.25f;
  static constexpr float FADE_DURATION = 1.45f;
  static constexpr float MAX_RADIUS = 14.0f;
  float age = 0.0f;
  bool finished = false;

  static float smoothStep(float value) {
    value = constrain(value, 0.0f, 1.0f);
    return value * value * (3.0f - 2.0f * value);
  }

 public:
  void init() override {
    state = state_t::RUNNING;
    age = -getAudioLeadSeconds();
    finished = false;
    setMotionBlur(0);
  }

  void draw(float dt) override {
    if (finished) return;
    age += dt;
    if (age < 0.0f) return;

    float radius = MAX_RADIUS;
    uint8_t brightness = 255;
    if (age < GROW_DURATION) {
      const float progress = smoothStep(age / GROW_DURATION);
      radius = 0.15f + progress * (MAX_RADIUS - 0.15f);
    } else if (age > GROW_DURATION + HOLD_DURATION) {
      const float fade = (age - GROW_DURATION - HOLD_DURATION) / FADE_DURATION;
      brightness = static_cast<uint8_t>(255.0f * (1.0f - constrain(fade, 0.0f, 1.0f)));
    }

    const Vector3 center(8.0f, 8.0f, 8.0f);
    for (uint8_t x = 0; x < Display::width; ++x) {
      for (uint8_t y = 0; y < Display::height; ++y) {
        for (uint8_t z = 0; z < Display::depth; ++z) {
          const Vector3 point(x, y, z);
          const Vector3 offset = point - center;
          if (offset.magnitude() <= radius)
            voxel(x, y, z, Color::WHITE.scaled(brightness));
        }
      }
    }

    if (age >= GROW_DURATION + HOLD_DURATION + FADE_DURATION)
      finished = true;
  }
};

#endif
