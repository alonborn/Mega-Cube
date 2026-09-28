#ifndef LIGHTCHASE_H
#define LIGHTCHASE_H

#include "Animation.h"

float getAudioLeadSeconds();

class LightChase : public Animation {
 private:
  static constexpr float ORBIT_RADIUS = 7.1f;
  static constexpr float BALL_RADIUS = 1.45f;
  static constexpr float TRACK_LENGTH = 123.4f;
  static constexpr float EXPLOSION_LENGTH = 3.0f;
  static constexpr float ORBIT_GAP = 180.0f;
  static const uint8_t DEBRIS_COUNT = 240;

  Vector3 direction;
  Vector3 axis;
  Vector3 targetAxis;
  Particle debris[2][DEBRIS_COUNT];
  uint8_t debrisBank = 1;
  float age = 0.0f;
  float axisAge = 0.0f;
  float axisDuration = 3.0f;
  uint8_t nextExplosion = 0;

  static float smooth(float value) {
    value = constrain(value, 0.0f, 1.0f);
    return value * value * (3.0f - 2.0f * value);
  }

  static float blend(float from, float to, float t) {
    return from + (to - from) * t;
  }

  Vector3 randomUnitVector() {
    Vector3 value(noise.nextRandom(-1.0f, 1.0f),
                  noise.nextRandom(-1.0f, 1.0f),
                  noise.nextRandom(-1.0f, 1.0f));
    if (value.magnitude() < 0.15f) value = Vector3(0.3f, 0.8f, 0.5f);
    return value.normalized();
  }

  Vector3 randomOrbitAxis() {
    Vector3 candidate = randomUnitVector();
    candidate -= direction * candidate.dot(direction);
    if (candidate.magnitude() < 0.2f) {
      candidate = direction.cross(Vector3(0, 1, 0));
      if (candidate.magnitude() < 0.2f)
        candidate = direction.cross(Vector3(1, 0, 0));
    }
    return candidate.normalized();
  }

  void updateAxis(float dt) {
    axisAge += dt;
    if (axisAge >= axisDuration) {
      axisAge = 0.0f;
      axisDuration = noise.nextRandom(2.0f, 4.5f);
      targetAxis = randomOrbitAxis();
    }

    const float blendAmount = min(1.0f, dt * 0.8f);
    axis += (targetAxis - axis) * blendAmount;
    if (axis.magnitude() < 0.05f) axis = targetAxis;
    axis.normalize();

    direction -= axis * direction.dot(axis);
    if (direction.magnitude() < 0.1f)
      direction = axis.cross(Vector3(0, 1, 0));
    direction.normalize();
  }

  float speedAt(float time) const {
    if (time < 0.0f) return 0.0f;
    if (time < 18.0f) {
      if (time < 3.0f) return blend(0.0f, 18.0f, smooth(time / 3.0f));
      return blend(18.0f, 95.0f, smooth((time - 3.0f) / 15.0f));
    }
    if (time < 50.0f) {
      const float progress = (time - 18.0f) / 32.0f;
      return blend(95.0f, 1200.0f, progress * progress);
    }
    if (time < 54.7f) return blend(1200.0f, 1550.0f, smooth((time - 50.0f) / 4.7f));
    if (time < 56.0f) return 0.0f;
    if (time < 90.0f) return blend(1200.0f, 1550.0f, smooth((time - 56.0f) / 34.0f));
    if (time < 101.0f) return 0.0f;
    if (time < 108.0f) return blend(300.0f, 1550.0f, smooth((time - 101.0f) / 7.0f));
    if (time < 109.0f) return blend(1550.0f, 1050.0f, smooth(time - 108.0f));
    if (time < 115.0f) return blend(1050.0f, 190.0f, smooth((time - 109.0f) / 6.0f));
    return 190.0f;
  }

  float ballBrightnessAt(float time) const {
    if (time < 0.0f) return 0.0f;
    if (time < 4.0f) return smooth(time / 4.0f);
    if (time < 54.0f) return 1.0f;
    if (time < 54.7f) return 1.0f - smooth((time - 54.0f) / 0.7f);
    if (time < 56.0f) return 0.0f;
    if (time < 57.0f) return smooth(time - 56.0f);
    if (time < 90.0f) return 1.0f;
    if (time < 90.7f) return 1.0f - smooth((time - 90.0f) / 0.7f);
    if (time < 101.0f) return 0.0f;
    if (time < 101.8f) return smooth((time - 101.0f) / 0.8f);
    if (time < 115.0f) return 1.0f;
    if (time < 119.0f) return 1.0f - smooth((time - 115.0f) / 4.0f);
    return 0.0f;
  }

  void drawBall(const Vector3 &center, const Color &color) {
    const int8_t minX = floorf(center.x - BALL_RADIUS);
    const int8_t maxX = ceilf(center.x + BALL_RADIUS);
    const int8_t minY = floorf(center.y - BALL_RADIUS);
    const int8_t maxY = ceilf(center.y + BALL_RADIUS);
    const int8_t minZ = floorf(center.z - BALL_RADIUS);
    const int8_t maxZ = ceilf(center.z + BALL_RADIUS);

    for (int8_t x = minX; x <= maxX; ++x) {
      for (int8_t y = minY; y <= maxY; ++y) {
        for (int8_t z = minZ; z <= maxZ; ++z) {
          const Vector3 point(x, y, z);
          const float distance = (point - center).magnitude();
          if (distance > BALL_RADIUS) continue;

          const float level = 0.68f + 0.32f * (1.0f - distance / BALL_RADIUS);
          voxel_add(point, color.scaled(static_cast<uint8_t>(255.0f * level)));
        }
      }
    }
  }

  void drawPulse(float time, float start, const Color &color) {
    const float elapsed = time - start;
    if (elapsed < 0.0f || elapsed >= 0.4f) return;

    float envelope;
    if (elapsed < 0.10f) {
      envelope = smooth(elapsed / 0.10f);
    } else if (elapsed < 0.16f) {
      envelope = 1.0f;
    } else {
      envelope = 1.0f - smooth((elapsed - 0.16f) / 0.24f);
    }

    const uint8_t intensity = static_cast<uint8_t>(8.0f * envelope);
    const Color pulse = color.scaled(intensity);
    for (uint8_t x = 0; x < Display::width; ++x) {
      for (uint8_t y = 0; y < Display::height; ++y) {
        for (uint8_t z = 0; z < Display::depth; ++z) {
          voxel_add(Vector3(x - CX, y - CY, z - CZ), pulse);
        }
      }
    }
  }

  void createDebris() {
    debrisBank = (debrisBank + 1) % 2;
    for (uint8_t i = 0; i < DEBRIS_COUNT; ++i) {
      const Vector3 velocity = randomUnitVector() * noise.nextRandom(2.8f, 8.0f);
      debris[debrisBank][i] = Particle(Vector3(0, 0, 0), velocity,
                            static_cast<uint8_t>(random(0, 72)), 1.0f,
                            noise.nextRandom(2.7f, 3.8f));
    }
  }

  void drawDebris(float dt) {
    for (uint8_t bank = 0; bank < 2; ++bank) {
      for (uint8_t i = 0; i < DEBRIS_COUNT; ++i) {
        Particle &particle = debris[bank][i];
      if (particle.brightness <= 0.0f) continue;

      particle.move(dt);
      particle.brightness =
          max(0.0f, particle.brightness - dt / particle.seconds);
      Color color(particle.hue, LavaPalette);
      if (random(0, 10) == 0) color = Color::WHITE;
        voxel_add(particle.position,
                  color.scaled(static_cast<uint8_t>(particle.brightness * 255.0f)));
      }
    }
  }

  void drawExplosion(float time, float eventTime) {
    const float elapsed = time - eventTime;
    if (elapsed < 0.0f || elapsed >= EXPLOSION_LENGTH) return;

    const float progress = elapsed / EXPLOSION_LENGTH;
    const float radius = 0.7f + progress * 12.5f;
    const float tail = 1.65f - progress * 0.55f;
    const float fade = (1.0f - smooth(progress)) * 0.92f;
    const Color blast = progress < 0.22f ? Color(255, 240, 205)
                                         : Color(255, 72, 12);

    for (uint8_t x = 0; x < Display::width; ++x) {
      for (uint8_t y = 0; y < Display::height; ++y) {
        for (uint8_t z = 0; z < Display::depth; ++z) {
          const float dx = x - CX;
          const float dy = y - CY;
          const float dz = z - CZ;
          const float distanceSquared = dx * dx + dy * dy + dz * dz;
          const float inner = max(0.0f, radius - tail);
          const float outer = radius + tail;
          if (distanceSquared < inner * inner || distanceSquared > outer * outer)
            continue;

          const float distance = sqrtf(distanceSquared);
          const float shell = 1.0f - fabsf(distance - radius) / tail;
          const float intensity = fade * shell * 0.92f;
          voxel_add(Vector3(x - CX, y - CY, z - CZ),
                    blast.scaled(static_cast<uint8_t>(255.0f * intensity)));
        }
      }
    }

    if (progress < 0.16f) {
      const uint8_t flash = static_cast<uint8_t>(255.0f * (1.0f - progress / 0.16f));
      radiate4(Vector3(0, 0, 0), Color(255, 220, 170).scaled(flash), 3.5f);
    }
  }

 public:
  void init() override {
    state = state_t::RUNNING;
    age = -getAudioLeadSeconds();
    direction = Vector3(0.72f, 0.25f, 0.64f).normalized();
    axis = Vector3(0.2f, 1.0f, 0.35f).normalized();
    axis.normalize();
    direction -= axis * direction.dot(axis);
    direction.normalize();
    targetAxis = randomOrbitAxis();
    axisAge = 0.0f;
    axisDuration = noise.nextRandom(2.0f, 4.0f);
    nextExplosion = 0;
    debrisBank = 1;
    for (uint8_t bank = 0; bank < 2; ++bank)
      for (uint8_t i = 0; i < DEBRIS_COUNT; ++i)
        debris[bank][i].brightness = 0.0f;
    setMotionBlur(0);
  }

  void draw(float dt) override {
    age += dt;
    if (age >= TRACK_LENGTH) return;

    updateAxis(dt);
    const float angularSpeed = speedAt(age);
    direction = Quaternion(angularSpeed * dt, axis).rotate(direction);
    direction -= axis * direction.dot(axis);
    direction.normalize();

    const float ballBrightness = ballBrightnessAt(age);
    if (ballBrightness > 0.001f) {
      const Vector3 first = direction * ORBIT_RADIUS;
      const Vector3 second =
          Quaternion(ORBIT_GAP, axis).rotate(direction) * ORBIT_RADIUS;
      const uint8_t level = static_cast<uint8_t>(255.0f * ballBrightness);
      drawBall(first, Color(20, 215, 255).scaled(level));
      drawBall(second, Color(255, 35, 135).scaled(level));
    }

    static const float explosionTimes[] = {92.0f, 96.0f, 100.0f, 103.0f};
    while (nextExplosion < 4 && age >= explosionTimes[nextExplosion] + EXPLOSION_LENGTH * 0.5f) {
      createDebris();
      ++nextExplosion;
    }

    drawPulse(age, 3.75f, Color(15, 70, 255));
    drawPulse(age, 11.75f, Color(255, 18, 42));
    drawDebris(dt);
    drawExplosion(age, 92.0f);
    drawExplosion(age, 96.0f);
    drawExplosion(age, 100.0f);
    drawExplosion(age, 103.0f);
  }
};

#endif
