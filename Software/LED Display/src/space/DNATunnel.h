#ifndef DNATUNNEL_H
#define DNATUNNEL_H

#include "Animation.h"

class DNATunnel : public Animation {
 private:
  static constexpr float DURATION = 30.0f;
  static const uint8_t TUNNEL_RINGS = 9;
  static const uint8_t RING_POINTS = 20;
  static const uint8_t RIBS = 6;

  float age = 0.0f;

  Vector3 orientTunnel(Vector3 point) {
    const float cycle = fmodf(age, DURATION) * TWO_PI / DURATION;

    // Integer harmonics vary the speed while meeting seamlessly at the loop.
    const float yaw = 0.72f * sinf(cycle) + 0.28f * sinf(cycle * 3.0f);
    const float pitch = 0.62f * sinf(cycle * 2.0f + 1.3f) +
                        0.25f * sinf(cycle * 5.0f);
    const float roll = cycle * 2.0f + 0.35f * sinf(cycle * 4.0f);

    const float cy = cosf(yaw);
    const float sy = sinf(yaw);
    const float cp = cosf(pitch);
    const float sp = sinf(pitch);
    const float cr = cosf(roll);
    const float sr = sinf(roll);

    const float x1 = point.x * cy + point.z * sy;
    const float z1 = -point.x * sy + point.z * cy;
    const float y2 = point.y * cp - z1 * sp;
    const float z2 = point.y * sp + z1 * cp;
    return Vector3(x1 * cr - y2 * sr, x1 * sr + y2 * cr, z2);
  }

  Vector3 tunnelPoint(float depth, float angle) {
    const float normalized = (depth + 8.0f) / 16.0f;
    const float radius = 1.15f + 5.75f * normalized;
    return orientTunnel(
        Vector3(cosf(angle) * radius, sinf(angle) * radius, depth));
  }

 public:
  void init() override {
    state = state_t::RUNNING;
    age = 0.0f;
    setMotionBlur(70);
  }

  void draw(float dt) override {
    age += dt;
    const float cycle = fmodf(age, DURATION) * TWO_PI / DURATION;
    const float travel = fmodf(cycle * (16.0f * 12.0f) / TWO_PI, 16.0f);
    const float twist = cycle * 4.0f;

    // Rings grow as they rush from the vanishing point toward the viewer.
    for (uint8_t ring = 0; ring < TUNNEL_RINGS; ++ring) {
      float depth = -8.0f + ring * (16.0f / TUNNEL_RINGS) + travel;
      if (depth > 8.0f) depth -= 16.0f;

      const float phase = twist + depth * 0.22f;
      const float brightness = 0.28f + 0.72f * ((depth + 8.0f) / 16.0f);
      Color ringColor(ring * (256 / TUNNEL_RINGS), RainbowGradientPalette);
      ringColor.scale(static_cast<uint8_t>(255 * brightness));

      Vector3 first = tunnelPoint(depth, phase);
      Vector3 previous = first;
      for (uint8_t point = 1; point < RING_POINTS; ++point) {
        const float angle = phase + point * TWO_PI / RING_POINTS;
        Vector3 current = tunnelPoint(depth, angle);
        line(previous, current, ringColor);
        previous = current;
      }
      line(previous, first, ringColor);
    }

    // Twisted ribs join the rings into a continuous hollow tube.
    for (uint8_t rib = 0; rib < RIBS; ++rib) {
      Vector3 previous;
      for (uint8_t step = 0; step <= 16; ++step) {
        const float depth = -8.0f + step;
        const float angle = rib * TWO_PI / RIBS + twist + depth * 0.22f;
        Vector3 current = tunnelPoint(depth, angle);
        if (step > 0) line(previous, current, Color(70, 12, 125));
        previous = current;
      }
    }

    radiate4(orientTunnel(Vector3(0, 0, -7.8f)),
             Color(190, 255, 255), 0.65f);

    if (age >= DURATION) state = state_t::INACTIVE;
  }
};

#endif
