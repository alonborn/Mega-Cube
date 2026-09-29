#ifndef UNIVERSAL_H
#define UNIVERSAL_H

#include "Animation.h"

void sendAnimationEvent(const char* event);
float getAudioLeadSeconds();

static const uint8_t UNIVERSAL_FONT[9][5] = {
    {0x5, 0x5, 0x5, 0x5, 0x7},  // U
    {0x5, 0x7, 0x7, 0x7, 0x5},  // N
    {0x7, 0x2, 0x2, 0x2, 0x7},  // I
    {0x5, 0x5, 0x5, 0x5, 0x2},  // V
    {0x7, 0x4, 0x6, 0x4, 0x7},  // E
    {0x6, 0x5, 0x6, 0x5, 0x5},  // R
    {0x7, 0x4, 0x7, 0x1, 0x7},  // S
    {0x2, 0x5, 0x7, 0x5, 0x5},  // A
    {0x4, 0x4, 0x4, 0x4, 0x7},  // L
};

class Universal : public Animation {
 private:
  static const uint8_t STAR_COUNT = 45;
  static constexpr float TRACK_DURATION = 22.267f;
  static constexpr float CENTER = 0.0f;
  static constexpr float EARTH_RADIUS = 5.25f;

  Vector3 stars[STAR_COUNT];
  float age = 0.0f;

  static bool isLand(const Vector3& p) {
    const float rough = 0.035f * sinf(p.x * 19.0f + p.z * 13.0f) *
                        cosf(p.y * 17.0f - p.z * 11.0f);
    return p.dot(Vector3(-0.43f, 0.43f, 0.79f)) > 0.76f + rough ||
           p.dot(Vector3(-0.29f, -0.42f, 0.86f)) > 0.82f + rough ||
           p.dot(Vector3(0.20f, 0.55f, 0.81f)) > 0.82f + rough ||
           p.dot(Vector3(0.29f, -0.18f, 0.94f)) > 0.78f + rough ||
           p.dot(Vector3(0.76f, 0.34f, 0.55f)) > 0.76f + rough ||
           p.dot(Vector3(0.70f, -0.57f, 0.43f)) > 0.84f + rough ||
           p.dot(Vector3(0.28f, 0.25f, -0.93f)) > 0.80f + rough ||
           p.dot(Vector3(-0.60f, -0.20f, -0.77f)) > 0.84f + rough;
  }

  void drawStars() {
    for (uint8_t i = 0; i < STAR_COUNT; ++i) {
      const uint8_t brightness = 70 + (i * 37) % 110;
      voxel_add(stars[i], Color(155, 185, 255).scaled(brightness));
    }
  }

  void drawEarth(float time) {
    const Quaternion spin(time * 10.0f, Vector3::Y);
    const Vector3 light = Vector3(-0.55f, 0.42f, 0.72f).normalized();
    const float outer2 = EARTH_RADIUS * EARTH_RADIUS;
    const float innerRadius = EARTH_RADIUS - 1.05f;

    for (int8_t x = -5; x <= 5; ++x) {
      for (int8_t y = -5; y <= 5; ++y) {
        for (int8_t z = -5; z <= 5; ++z) {
          const Vector3 local(x, y, z);
          const float distance2 = local.dot(local);
          if (distance2 > outer2 || distance2 < innerRadius * innerRadius)
            continue;

          const float distance = sqrtf(distance2);
          const Vector3 normal = local / distance;
          const Vector3 worldNormal = spin.rotate(normal);
          const float daylight = max(0.12f, worldNormal.dot(light));
          const Vector3 point =
              Vector3(CENTER, CENTER, CENTER) + spin.rotate(local);
          Color color;

          if (isLand(normal)) {
            color = Color(35, 145, 48).scaled(70 + daylight * 185);
            if (normal.y < -0.55f)
              color = Color(155, 128, 68).scaled(65 + daylight * 165);
          } else {
            color = Color(8, 48, 155).scaled(55 + daylight * 200);
          }

          if (distance > EARTH_RADIUS - 0.45f && daylight > 0.25f)
            color = Color(255, 105, 28).scaled(70 + daylight * 185);
          voxel_add(point, color);
        }
      }
    }
  }

  void drawTitle(float time) {
    const float orbit = time * 24.0f;
    const float step = 180.0f / 36.0f;

    for (uint8_t letter = 0; letter < 9; ++letter) {
      for (uint8_t row = 0; row < 5; ++row) {
        for (uint8_t column = 0; column < 3; ++column) {
          if (!(UNIVERSAL_FONT[letter][row] & (1 << (2 - column)))) continue;

          const float index = letter * 4.0f + column;
          const float angle = orbit + (index - 17.5f) * step;
          const float radians = angle * DEG_TO_RAD;
          const float y = (2.0f - row) * 0.88f;
          const Vector3 radial(sinf(radians), 0, cosf(radians));
          const Vector3 center(0, y, 0);
          const Vector3 inner = Vector3(CENTER, CENTER, CENTER) + center +
                                radial * 5.05f;
          const Vector3 outer = Vector3(CENTER, CENTER, CENTER) + center +
                                radial * 6.55f;
          line(inner, outer, Color(255, 224, 155));
          voxel_add(outer, Color(255, 250, 224));
        }
      }
    }
  }

 public:
  void init() override {
    state = state_t::RUNNING;
    age = -getAudioLeadSeconds();
    sendAnimationEvent("UNIV_START");
    setMotionBlur(0);

    for (uint8_t i = 0; i < STAR_COUNT; ++i) {
      stars[i] = Vector3(random(-75, 76) * 0.1f,
                        random(-75, 76) * 0.1f,
                        random(-75, 76) * 0.1f);
      const Vector3 offset = stars[i] - Vector3(CENTER, CENTER, CENTER);
      if (offset.magnitude() < EARTH_RADIUS + 1.0f)
        stars[i].z = random(0, 2) ? -7.5f : 7.5f;
    }
  }

  void draw(float dt) override {
    age += dt;
    if (age < 0.0f) return;

    const float visualTime = min(age, TRACK_DURATION);
    drawStars();
    drawEarth(visualTime);
    drawTitle(visualTime);
  }
};

#endif
