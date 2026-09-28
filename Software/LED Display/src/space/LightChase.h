#ifndef LIGHTCHASE_H
#define LIGHTCHASE_H

#include "Animation.h"

class LightChase : public Animation {
 private:
  static constexpr float ORBIT_RADIUS = 7.1f;
  static constexpr float BALL_RADIUS = 1.45f;

  Vector3 direction;
  Vector3 axis;
  Vector3 targetAxis;
  float speed = 0.0f;
  float targetSpeed = 0.0f;
  float phaseAge = 0.0f;
  float phaseDuration = 1.0f;
  bool leavingStop = false;

  Vector3 randomUnitVector() {
    Vector3 value(noise.nextRandom(-1.0f, 1.0f),
                  noise.nextRandom(-1.0f, 1.0f),
                  noise.nextRandom(-1.0f, 1.0f));
    if (value.magnitude() < 0.15f) value = Vector3(0.3f, 0.8f, 0.5f);
    return value.normalized();
  }

  void chooseMotion() {
    phaseAge = 0.0f;
    targetAxis = randomUnitVector();

    if (leavingStop) {
      // Every slowdown is followed immediately by a long, fast escape.
      targetSpeed = noise.nextRandom(460.0f, 680.0f);
      if (speed > 0.0f) targetSpeed = -targetSpeed;
      phaseDuration = noise.nextRandom(4.0f, 7.0f);
      leavingStop = false;
      return;
    }

    const uint8_t choice = random(0, 100);
    if (choice < 92) {
      // Most of the time they keep racing, only changing orbit and pace.
      targetSpeed = noise.nextRandom(420.0f, 680.0f);
      if (random(0, 100) < 14) targetSpeed = -targetSpeed;
      phaseDuration = noise.nextRandom(3.8f, 7.0f);
    } else if (choice < 97) {
      // A brief, still energetic transition before the next sprint.
      targetSpeed = noise.nextRandom(260.0f, 360.0f);
      if (random(0, 2)) targetSpeed = -targetSpeed;
      phaseDuration = noise.nextRandom(0.45f, 0.85f);
    } else {
      // Rare dramatic brake used to pick a completely different direction.
      targetSpeed = noise.nextRandom(0.0f, 10.0f);
      phaseDuration = noise.nextRandom(0.25f, 0.55f);
      leavingStop = true;
    }
  }

  void updateMotion(float dt) {
    phaseAge += dt;
    if (phaseAge >= phaseDuration) chooseMotion();

    const float axisBlend = min(1.0f, dt * (fabsf(speed) < 25.0f ? 3.8f : 1.4f));
    axis += (targetAxis - axis) * axisBlend;
    if (axis.magnitude() < 0.05f) axis = targetAxis;
    axis.normalize();

    const float acceleration = fabsf(targetSpeed) < 15.0f ? 520.0f : 390.0f;
    const float speedDelta = targetSpeed - speed;
    const float maxChange = acceleration * dt;
    speed += constrain(speedDelta, -maxChange, maxChange);

    direction = Quaternion(speed * dt, axis).rotate(direction);
    direction.normalize();
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

          const float level =
              0.68f + 0.32f * (1.0f - distance / BALL_RADIUS);
          voxel_add(point, color.scaled(
                               static_cast<uint8_t>(255.0f * level)));
        }
      }
    }
  }

 public:
  void init() override {
    state = state_t::RUNNING;
    direction = Vector3(0.72f, 0.25f, 0.64f).normalized();
    axis = Vector3(0.2f, 1.0f, 0.35f).normalized();
    targetAxis = axis;
    speed = 460.0f;
    targetSpeed = speed;
    leavingStop = false;
    phaseAge = 0.0f;
    phaseDuration = 1.4f;
    setMotionBlur(0);
  }

  void draw(float dt) override {
    updateMotion(dt);

    const Vector3 first = direction * ORBIT_RADIUS;
    const Vector3 second = -first;
    drawBall(first, Color(20, 215, 255));
    drawBall(second, Color(255, 35, 135));
  }
};

#endif
