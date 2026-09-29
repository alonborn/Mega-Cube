#ifndef DELOREANFLIGHT_H
#define DELOREANFLIGHT_H

#include "Animation.h"

class DeLoreanFlight : public Animation {
 private:
  static const uint8_t STAR_COUNT = 100;

  struct Star {
    float x;
    float y;
    float depth;
    uint8_t hue;
  };

  Star stars[STAR_COUNT];
  float age = 0.0f;

  static Vector3 rotateScene(const Vector3 &point) {
    return Quaternion(270.0f, Vector3::Y).rotate(point);
  }

  static float smooth(float value) {
    value = constrain(value, 0.0f, 1.0f);
    return value * value * (3.0f - 2.0f * value);
  }

  Vector3 projectStar(const Star &star, float depth) const {
    return Vector3(depth, star.y * 7.5f, star.x * 7.5f);
  }

  void drawStars(float dt) {
    const float flightAge = max(0.0f, age - 11.0f);
    const float acceleration = smooth(flightAge / 34.0f);
    const float speed = 0.45f + acceleration * 8.5f;
    const float tailLength = 0.12f + acceleration * 2.7f;
    const uint8_t brightness = static_cast<uint8_t>(65.0f + acceleration * 190.0f);

    for (uint8_t i = 0; i < STAR_COUNT; ++i) {
      Star &star = stars[i];
      const float oldDepth = star.depth;
      star.depth += speed * dt;

      if (star.depth > 7.5f) {
        star.depth = -7.5f;
        star.x = noise.nextRandom(-1.0f, 1.0f);
        star.y = noise.nextRandom(-1.0f, 1.0f);
        star.hue = random(0, 256);
        continue;
      }

      const Vector3 head = projectStar(star, star.depth);
      const Vector3 tail = projectStar(star, max(oldDepth, star.depth - tailLength));
      Color color = Color(star.hue, RainbowGradientPalette).scaled(brightness);
      line(rotateScene(tail), rotateScene(head), color.scaled(85));
      voxel_add(rotateScene(head), color);
    }
  }

  void carVoxel(const Vector3 &point, const Color &color,
                const Vector3 &origin, float pitch, float roll,
                uint8_t brightness) {
    Vector3 rotated = Quaternion(roll, Vector3(0, 0, 1)).rotate(point);
    rotated = Quaternion(pitch, Vector3(1, 0, 0)).rotate(rotated);
    voxel_add(rotateScene(origin + rotated), color.scaled(brightness));
  }

  void drawWheel(float x, float y, float z, const Vector3 &origin,
                 float pitch, float roll, uint8_t brightness) {
    for (int8_t dx = -1; dx <= 1; ++dx) {
      for (int8_t dy = -1; dy <= 1; ++dy) {
        for (int8_t dz = -1; dz <= 1; ++dz) {
          const float radius = dx * dx + dy * dy + dz * dz;
          if (radius > 2.1f) continue;

          Color tire(24, 42, 62);
          if (radius < 0.6f) tire = Color(75, 190, 255);
          carVoxel(Vector3(x + dx * 0.62f, y + dy * 0.62f,
                           z + dz * 0.62f), tire, origin, pitch, roll,
                   brightness);
        }
      }
    }
  }

  void drawCar(float baseY, float wobbleX, float wobbleZ,
               float pitch, float roll, uint8_t brightness) {
    const Vector3 origin(wobbleX, baseY, wobbleZ);
    const Color body(155, 190, 215);
    const Color edge(225, 245, 255);
    const Color glass(35, 145, 220);
    const Color dimGlass(16, 72, 130);

    for (int8_t z = -4; z <= 4; ++z) {
      const int8_t halfWidth = abs(z) > 2 ? 1 : 2;
      for (int8_t x = -halfWidth; x <= halfWidth; ++x) {
        carVoxel(Vector3(x, -0.18f, z), body, origin, pitch, roll, brightness);
      }
      carVoxel(Vector3(-halfWidth, -0.48f, z), edge, origin, pitch, roll,
               brightness);
      carVoxel(Vector3(halfWidth, -0.48f, z), edge, origin, pitch, roll,
               brightness);
    }

    for (int8_t z = -4; z <= 4; ++z) {
      const int8_t halfWidth = abs(z) > 2 ? 1 : 2;
      for (int8_t x = -halfWidth; x <= halfWidth; ++x) {
        carVoxel(Vector3(x, 0.12f, z), body, origin, pitch, roll, brightness);
      }
    }

    for (int8_t z = -1; z <= 1; ++z) {
      for (int8_t y = 0; y <= 1; ++y) {
        carVoxel(Vector3(-1.55f, 0.55f + y * 0.72f, z), edge,
                 origin, pitch, roll, brightness);
        carVoxel(Vector3(1.55f, 0.55f + y * 0.72f, z), edge,
                 origin, pitch, roll, brightness);
      }
      carVoxel(Vector3(-0.9f, 1.78f, z), edge, origin, pitch, roll, brightness);
      carVoxel(Vector3(0.9f, 1.78f, z), edge, origin, pitch, roll, brightness);
    }

    for (int8_t x = -1; x <= 1; ++x) {
      carVoxel(Vector3(x, 1.05f, 1.45f), glass, origin, pitch, roll, brightness);
      carVoxel(Vector3(x, 1.55f, 0.95f), glass, origin, pitch, roll, brightness);
      carVoxel(Vector3(x, 1.05f, -1.45f), dimGlass, origin, pitch, roll,
               brightness);
      carVoxel(Vector3(x, 1.78f, -1.0f), edge, origin, pitch, roll, brightness);
    }

    for (int8_t z = -1; z <= 1; ++z) {
      for (int8_t x = -1; x <= 1; ++x) {
        carVoxel(Vector3(x, 1.2f, z), dimGlass, origin, pitch, roll, brightness);
      }
    }

    for (int8_t side = -1; side <= 1; side += 2) {
      drawWheel(side * 1.75f, -0.45f, -2.65f, origin, pitch, roll, brightness);
      drawWheel(side * 1.75f, -0.45f, 2.65f, origin, pitch, roll, brightness);
    }

    for (int8_t side = -1; side <= 1; side += 2) {
      carVoxel(Vector3(side * 1.15f, 0.35f, 4.0f), Color::WHITE,
               origin, pitch, roll, brightness);
      carVoxel(Vector3(side * 1.15f, 0.35f, -4.0f), Color(255, 30, 55),
               origin, pitch, roll, brightness);
    }

    carVoxel(Vector3(0, 0.8f, -3.7f), Color(40, 200, 255),
             origin, pitch, roll, brightness);
    carVoxel(Vector3(0, 0.8f, -3.25f), Color(175, 235, 255),
             origin, pitch, roll, brightness);
  }

 public:
  void init() override {
    state = state_t::RUNNING;
    age = 0.0f;
    for (uint8_t i = 0; i < STAR_COUNT; ++i) {
      stars[i] = {noise.nextRandom(-1.0f, 1.0f),
                  noise.nextRandom(-1.0f, 1.0f),
                  noise.nextRandom(-7.5f, 7.5f),
                  static_cast<uint8_t>(random(0, 256))};
    }
    setMotionBlur(0);
  }

  void draw(float dt) override {
    age += dt;

    const float rise = smooth(age / 10.0f);
    const float instability = 1.0f - rise;
    const float baseY = -6.5f + rise * 6.0f;
    const float wobbleX = sinf(age * 1.9f) * 1.5f * instability;
    const float wobbleZ = cosf(age * 2.25f) * 0.9f * instability;
    const float wobbleY = sinf(age * 2.65f) * 0.55f * instability;
    const float pitch = sinf(age * 2.2f) * 11.0f * instability;
    const float roll = cosf(age * 1.75f) * 13.0f * instability;
    const uint8_t carBrightness =
        static_cast<uint8_t>(255.0f * smooth(age / 0.75f));

    drawStars(dt);
    drawCar(baseY + wobbleY, wobbleX, wobbleZ, pitch, roll, carBrightness);
  }
};

#endif
