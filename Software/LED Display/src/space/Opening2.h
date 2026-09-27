#ifndef OPENING2_H
#define OPENING2_H
#include "Animation.h"

float getAudioLeadSeconds();

class Opening2 : public Animation {
 private:
  static constexpr float C = 7.5f;
  float age = 0.0f;

  static float unit(float n) { return constrain(n, 0.0f, 1.0f); }
  static float ease(float n) {
    n = unit(n);
    return n * n * (3.0f - 2.0f * n);
  }
  static Color rainbow(float hue, uint8_t level = 255) {
    return Color(static_cast<uint8_t>(hue), RainbowGradientPalette).scaled(level);
  }
  static float distance(uint8_t x, uint8_t y, uint8_t z) {
    return Vector3(x - C, y - C, z - C).magnitude();
  }
  static void add(uint8_t x, uint8_t y, uint8_t z, const Color &color) {
    voxel(x, y, z, color);
  }

  void spark() {
    const float radius = 0.4f + 3.0f * ease(age / 0.35f);
    for (uint8_t x = 0; x < 16; ++x)
      for (uint8_t y = 0; y < 16; ++y)
        for (uint8_t z = 0; z < 16; ++z) {
          const float d = distance(x, y, z);
          if (d <= radius) {
            const uint8_t level =
                255.0f * unit(1.0f - d / (radius + 0.2f));
            add(x, y, z, Color(level, level, 255));
          }
        }
  }

  void scans() {
    const float p = (age - 0.35f) / 0.80f;
    const float tx = 8.0f * ease(p / 0.72f);
    const float ty = 8.0f * ease((p - 0.14f) / 0.72f);
    const float tz = 8.0f * ease((p - 0.28f) / 0.72f);
    for (uint8_t x = 0; x < 16; ++x)
      for (uint8_t y = 0; y < 16; ++y)
        for (uint8_t z = 0; z < 16; ++z) {
          if (fabsf(fabsf(x - C) - tx) < 0.65f) add(x, y, z, Color::CYAN);
          if (fabsf(fabsf(y - C) - ty) < 0.65f) add(x, y, z, Color::MAGENTA);
          if (fabsf(fabsf(z - C) - tz) < 0.65f) add(x, y, z, Color::YELLOW);
        }
  }

  void wave() {
    const float radius = 0.5f + 12.8f * ease((age - 1.15f) / 0.95f);
    for (uint8_t x = 0; x < 16; ++x)
      for (uint8_t y = 0; y < 16; ++y)
        for (uint8_t z = 0; z < 16; ++z) {
          const float d = distance(x, y, z);
          const float edge = fabsf(d - radius);
          if (edge < 1.15f)
            add(x, y, z, rainbow(d * 24.0f + age * 110.0f,
                255.0f * (1.0f - edge / 1.15f)));
        }
  }

  void burst() {
    static const int8_t dirs[][3] = {
        {1,0,0},{0,1,0},{0,0,1},{-1,0,0},{0,-1,0},{0,0,-1},
        {1,1,1},{-1,1,1},{1,-1,1},{1,1,-1},{-1,-1,1},
        {-1,1,-1},{1,-1,-1},{-1,-1,-1}};
    const float p = ease((age - 2.10f) / 0.75f);
    const float tip = 1.0f + p * 12.5f;
    const float tail = max(0.0f, tip - 5.0f);
    for (uint8_t i = 0; i < sizeof(dirs) / sizeof(dirs[0]); ++i) {
      Vector3 direction(dirs[i][0], dirs[i][1], dirs[i][2]);
      direction.normalize();
      line(Vector3(C, C, C) + direction * tail,
           Vector3(C, C, C) + direction * tip,
           rainbow(i * 19 + age * 90));
    }
    const float radius = 1.0f + p * 13.0f;
    for (uint8_t x = 0; x < 16; ++x)
      for (uint8_t y = 0; y < 16; ++y)
        for (uint8_t z = 0; z < 16; ++z)
          if (fabsf(distance(x, y, z) - radius) < 0.55f)
            add(x, y, z, Color::WHITE);
  }

  void drawFloor(float opacity) {
    for (uint8_t x = 0; x < 16; ++x)
      for (uint8_t z = 0; z < 16; ++z) {
        const float flow =
            sinf(x * 0.65f + age * 8.0f) +
            sinf(z * 0.75f - age * 6.4f) +
            sinf((x + z) * 0.38f + age * 4.7f);
        const uint8_t hue = static_cast<uint8_t>(
            age * 95.0f + x * 11.0f - z * 9.0f + flow * 35.0f);
        const float pulse = 0.5f + 0.5f *
            sinf(x * 0.42f - z * 0.51f + age * 9.0f + flow);
        const uint8_t level = static_cast<uint8_t>(
            (145.0f + pulse * 110.0f) * opacity);
        voxel(x, 0, z, rainbow(hue, level));
      }
  }

  void melt() {
    const float p = ease((age - 2.10f) / 0.95f);
    for (uint8_t x = 0; x < 16; ++x)
      for (uint8_t y = 0; y < 16; ++y)
        for (uint8_t z = 0; z < 16; ++z) {
          const uint8_t edges = (x == 0 || x == 15) +
                                (y == 0 || y == 15) +
                                (z == 0 || z == 15);
          if (edges < 2) continue;
          const uint8_t fallingY = static_cast<uint8_t>(
              constrain(y * (1.0f - p), 0.0f, 15.0f));
          const uint8_t hue = static_cast<uint8_t>(
              x * 9 + y * 13 + z * 17 + age * 80.0f);
          voxel(x, fallingY, z, rainbow(hue));
          if (fallingY > 0)
            voxel(x, fallingY - 1, z, rainbow(hue + 18, 150));
        }
    drawFloor(p);
  }

  void idleFlow() {
    drawFloor(1.0f);
  }

 public:
  void init() override {
    state = state_t::RUNNING;
    age = -getAudioLeadSeconds();
    setMotionBlur(0);
  }
  void draw(float dt) override {
    age += dt;
    if (age < 0.0f) return;
    if (age < 0.35f) spark();
    else if (age < 1.15f) scans();
    else if (age < 2.10f) wave();
    else if (age < 3.05f) melt();
    else idleFlow();
  }
};
#endif
