#ifndef HELIX2_H
#define HELIX2_H

#include "Animation.h"

class Helix2 : public Animation {
 private:
  static const uint8_t MAX_PASSES = 3;
  static constexpr float SPAWN_INTERVAL = 3.15f;

  struct Pass {
    bool active;
    uint8_t direction;
    float head;
    float phase;
    uint8_t hue;
  };

  Pass passes[MAX_PASSES];
  float spawnAge = 0.0f;
  uint8_t nextDirection = 0;

  static constexpr auto &settings = config.animation.helix;

  Vector3 orient(Vector3 point, uint8_t direction) {
    switch (direction) {
      case 1:
        return Quaternion(-90, Vector3(0, 0, 1)).rotate(point);
      case 2:
        return Quaternion(90, Vector3(1, 0, 0)).rotate(point);
      case 3:
        return Quaternion(180, Vector3(0, 0, 1)).rotate(point);
      case 4:
        return Quaternion(90, Vector3(0, 0, 1)).rotate(point);
      case 5:
        return Quaternion(-90, Vector3(1, 0, 0)).rotate(point);
      default:
        return point;
    }
  }

  void spawnPass() {
    for (uint8_t i = 0; i < MAX_PASSES; ++i) {
      if (passes[i].active) continue;
      passes[i].active = true;
      passes[i].direction = nextDirection;
      passes[i].head = 0.0f;
      passes[i].phase = noise.nextRandom(0.0f, TWO_PI);
      passes[i].hue = static_cast<uint8_t>((hue16 >> 8) +
                                           nextDirection * 43);
      nextDirection = (nextDirection + 1) % 6;
      return;
    }
  }

 public:
  void init() override {
    state = state_t::RUNNING;
    spawnAge = 0.0f;
    nextDirection = 0;
    hue16 = random(0, 256) << 8;
    for (uint8_t i = 0; i < MAX_PASSES; ++i) passes[i].active = false;
    spawnPass();
    setMotionBlur(145);
  }

  void draw(float dt) override {
    const float resolution = settings.resolution;
    const float trailLength = resolution * 0.78f;
    const float travelSpeed = resolution / 3.2f;
    const float phaseSpeed = settings.phase_speed;
    const float radius = settings.radius;
    const uint8_t brightness =
        static_cast<uint8_t>(settings.brightness * getBrightness());

    spawnAge += dt;
    hue16 += dt * settings.hue_speed * 110.0f;
    if (spawnAge >= SPAWN_INTERVAL) {
      spawnAge -= SPAWN_INTERVAL;
      spawnPass();
    }

    Quaternion tilt(settings.angle, Vector3(1, 0, 0));

    for (uint8_t i = 0; i < MAX_PASSES; ++i) {
      Pass &pass = passes[i];
      if (!pass.active) continue;

      pass.head += travelSpeed * dt;
      pass.phase += phaseSpeed * dt;
      const float tail = pass.head - trailLength;
      if (tail > resolution) {
        pass.active = false;
        continue;
      }

      const int16_t first =
          max(0, static_cast<int16_t>(ceilf(tail)));
      const int16_t last =
          min(static_cast<int16_t>(resolution),
              static_cast<int16_t>(floorf(pass.head)));

      for (int16_t sample = first; sample <= last; ++sample) {
        const float along = sample / resolution;
        const float angle = pass.phase + along * TWO_PI;
        const float xf = sinf(angle);
        const float zf = cosf(angle);
        Vector3 point =
            Vector3(xf, 2.0f * along - 1.0f, zf) * radius;
        point = orient(tilt.rotate(point), pass.direction);

        const float headFade =
            constrain((pass.head - sample) / 3.0f, 0.0f, 1.0f);
        const float tailFade =
            constrain((sample - tail) / 3.0f, 0.0f, 1.0f);
        const uint8_t level = static_cast<uint8_t>(
            brightness * min(headFade, tailFade));
        Color color(static_cast<uint8_t>(
                        pass.hue + sample * 3 + (hue16 >> 9)),
                    RainbowGradientPalette);
        radiate(point, color.scale(level), 1.0f);
      }
    }
  }
};

#endif
