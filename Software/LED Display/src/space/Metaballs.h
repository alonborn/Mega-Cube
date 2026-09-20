#ifndef METABALLS_H
#define METABALLS_H

#include "Animation.h"

class Metaballs : public Animation {
 private:
  static const uint8_t BALLS = 4;
  static constexpr float DURATION = 30.0f;
  static constexpr float THRESHOLD = 0.82f;

  float age = 0.0f;
  Vector3 centers[BALLS];
  Vector3 velocities[BALLS];
  float radii[BALLS] = {1.58f, 1.5f, 1.54f, 1.48f};
  Color colors[BALLS] = {Color::RED, Color::GREEN, Color::BLUE, Color::YELLOW};

  static void bounceAxis(float &position, float &velocity, float limit) {
    if (position > limit) {
      position = limit;
      velocity = -fabsf(velocity);
    } else if (position < -limit) {
      position = -limit;
      velocity = fabsf(velocity);
    }
  }

  void updatePhysics(float dt) {
    const uint8_t steps = max(1, static_cast<int>(ceilf(dt / 0.008f)));
    const float step = dt / steps;

    for (uint8_t substep = 0; substep < steps; ++substep) {
      for (uint8_t i = 0; i < BALLS; ++i) {
        centers[i] += velocities[i] * step;
        const float limit = 7.25f - radii[i];
        bounceAxis(centers[i].x, velocities[i].x, limit);
        bounceAxis(centers[i].y, velocities[i].y, limit);
        bounceAxis(centers[i].z, velocities[i].z, limit);
      }

      for (uint8_t i = 0; i < BALLS; ++i) {
        for (uint8_t j = i + 1; j < BALLS; ++j) {
          Vector3 delta = centers[j] - centers[i];
          const float distance = delta.magnitude();
          const float collision_distance = radii[i] + radii[j];
          if (distance >= collision_distance || distance < 0.001f) continue;

          const Vector3 normal = delta / distance;
          const float closing_speed = (velocities[j] - velocities[i]).dot(normal);
          if (closing_speed < 0.0f) {
            velocities[i] += normal * closing_speed;
            velocities[j] -= normal * closing_speed;
          }

          const Vector3 correction =
              normal * ((collision_distance - distance) * 0.5f);
          centers[i] -= correction;
          centers[j] += correction;
        }
      }
    }
  }

 public:
  void init() override {
    state = state_t::RUNNING;
    age = 0.0f;
    centers[0] = Vector3(-4.2f, -2.5f, -2.0f);
    centers[1] = Vector3(3.5f, 2.0f, 1.5f);
    centers[2] = Vector3(0.5f, 4.0f, -3.2f);
    centers[3] = Vector3(2.8f, -3.8f, 3.5f);
    velocities[0] = Vector3(8.8f, 6.1f, 7.6f);
    velocities[1] = Vector3(-7.8f, 8.6f, -6.5f);
    velocities[2] = Vector3(6.7f, -7.7f, 9.1f);
    velocities[3] = Vector3(-8.4f, -5.8f, 7.3f);
    setMotionBlur(80);
  }

  void draw(float dt) override {
    age += dt;
    updatePhysics(dt);

    for (uint8_t x = 0; x < Display::width; ++x) {
      for (uint8_t y = 0; y < Display::height; ++y) {
        for (uint8_t z = 0; z < Display::depth; ++z) {
          const Vector3 point(x - CX, y - CY, z - CZ);
          float field = 0.0f;
          float red = 0.0f;
          float green = 0.0f;
          float blue = 0.0f;

          for (uint8_t i = 0; i < BALLS; ++i) {
            const Vector3 offset = point - centers[i];
            const float distance_squared = offset.dot(offset);
            const float contribution =
                radii[i] * radii[i] / (distance_squared + 0.55f);
            field += contribution;
            red += colors[i].red * contribution;
            green += colors[i].green * contribution;
            blue += colors[i].blue * contribution;
          }

          if (field < THRESHOLD) continue;

          const float glow = min(1.0f, (field - THRESHOLD) * 1.4f + 0.22f);
          const float color_scale = glow / field;
          Color color(static_cast<uint8_t>(min(255.0f, red * color_scale)),
                      static_cast<uint8_t>(min(255.0f, green * color_scale)),
                      static_cast<uint8_t>(min(255.0f, blue * color_scale)));

          if (field > 3.6f) color.maximize(Color(90, 90, 90));
          voxel(x, y, z, color);
        }
      }
    }

    if (age >= DURATION) state = state_t::INACTIVE;
  }
};

#endif
