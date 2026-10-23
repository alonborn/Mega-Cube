#ifndef SINUS_H
#define SINUS_H

#include "Animation.h"

void sendAnimationEvent(const char* event);
float getAudioLeadSeconds();

class Sinus : public Animation {
 private:
  static constexpr float INTRO_DURATION = 18.0f;
  static constexpr float TRACK_DURATION = 150.442f;
  float x_min = -2;
  float x_max = 2;
  float z_min = -2;
  float z_max = 2;

  float radius;
  float resolution;
  float phase;
  float phase_speed;
  float age;

  static constexpr auto &settings = config.animation.sinus;

  static uint8_t auroraIntensity(float distance, float width, float shimmer) {
    if (distance >= width) return 0;
    float value = (1.0f - distance / width) * shimmer;
    value *= value;
    return static_cast<uint8_t>(255.0f * min(1.0f, value));
  }

  void drawIntro() {
    const float fade = constrain(age / 7.0f, 0.0f, 1.0f);
    const float auroraPhase = age * TWO_PI / 30.0f;
    const Quaternion rotation(age * 360.0f / INTRO_DURATION, Vector3::Y);
    setMotionBlur(205);

    for (uint8_t x = 0; x < Display::width; ++x) {
      for (uint8_t z = 0; z < Display::depth; ++z) {
        const float xf = x - CX;
        const float zf = z - CZ;
        const float center_green =
            CY + 4.6f * sinf(xf * 0.47f + 5.0f * auroraPhase +
                            0.8f * sinf(zf * 0.31f - 2.0f * auroraPhase)) +
            1.6f * sinf(zf * 0.58f + 3.0f * auroraPhase);
        const float center_blue =
            CY + 4.15f * sinf(xf * 0.39f - 4.0f * auroraPhase + 2.1f) +
            1.95f * sinf(zf * 0.36f + 3.0f * auroraPhase + 0.7f);
        const float shimmer_green =
            0.58f + 0.42f * (0.5f + 0.5f * sinf(zf * 0.78f - 10.0f * auroraPhase));
        const float shimmer_blue =
            0.50f + 0.38f * (0.5f + 0.5f * sinf(xf * 0.64f + 8.0f * auroraPhase));

        for (uint8_t y = 0; y < Display::height; ++y) {
          Color color = Color::BLACK;
          const uint8_t green = static_cast<uint8_t>(
              auroraIntensity(fabsf(y - center_green), 1.05f, shimmer_green) * fade);
          const uint8_t blue = static_cast<uint8_t>(
              auroraIntensity(fabsf(y - center_blue), 0.90f, shimmer_blue) * fade);
          color.maximize(Color(15, 255, 35).scaled(green));
          color.maximize(Color(25, 45, 255).scaled(blue));
          if (!color.isBlack()) {
            const Vector3 centered(x - CX, y - CY, z - CZ);
            voxel(rotation.rotate(centered), color);
          }
        }
      }
    }
  }

  void drawWave(float dt) {
    resolution = settings.resolution;
    hue16_speed = settings.hue_speed * 255;
    radius = settings.radius;
    phase_speed = settings.phase_speed;
    setMotionBlur(settings.motionBlur);
    const uint8_t brightness = settings.brightness * getBrightness();
    phase += dt * phase_speed;
    hue16 += dt * hue16_speed;

    Quaternion q = Quaternion(phase * 10.0f, Vector3(1, 1, 1));
    for (uint16_t x = 0; x <= resolution; x++) {
      float xprime = mapf(x, 0, resolution, x_min, x_max);
      for (uint16_t z = 0; z <= resolution; z++) {
        float zprime = mapf(z, 0, resolution, z_min, z_max);
        float y = sinf(phase + sqrtf(xprime * xprime + zprime * zprime));
        Vector3 point =
            Vector3(2 * (x / resolution) - 1, 2 * (z / resolution) - 1, y);
        point = q.rotate(point) * radius;
        Color c = Color((hue16 >> 8) + (int8_t)(y * 64), RainbowGradientPalette);
        radiate(point, c.scale(brightness), 1.0f);
      }
    }
  }

 public:
  void init() override {
    state = state_t::STARTING;
    age = -getAudioLeadSeconds();
    phase = 0;
    sendAnimationEvent("SINUS_START");
  }

  void draw(float dt) override {
    age += dt;
    if (age < 0.0f) return;
    if (age >= TRACK_DURATION) {
      state = state_t::INACTIVE;
      sendAnimationEvent("SINUS_STOP");
      return;
    }

    if (age < INTRO_DURATION) {
      drawIntro();
      return;
    }

    state = state_t::RUNNING;
    drawWave(dt);
  }
};

#endif
