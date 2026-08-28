#ifndef SPOTTEDSPHERE_H
#define SPOTTEDSPHERE_H

#include "Animation.h"

class SpottedSphere : public Animation {
 private:
  static const uint8_t SPOTS = 12;

  Vector3 spot_direction[SPOTS];
  Color spot_color[SPOTS];
  float angle;
  float time;

  static float smooth(float t) { return t * t * (3.0f - 2.0f * t); }

 public:
  void init() {
    state = state_t::RUNNING;
    angle = 0;
    time = 0;

    Vector3 directions[SPOTS] = {
        Vector3(1.0f, 0.1f, 0.0f),    Vector3(-0.8f, 0.4f, 0.5f),
        Vector3(0.3f, 0.9f, -0.5f),   Vector3(-0.2f, -0.8f, -0.6f),
        Vector3(0.7f, -0.4f, 0.8f),   Vector3(-0.9f, -0.1f, 0.2f),
        Vector3(0.1f, 0.5f, 1.0f),    Vector3(0.5f, -0.7f, -0.4f),
        Vector3(-0.4f, 0.8f, -0.9f),  Vector3(0.9f, 0.4f, -0.2f),
        Vector3(-0.6f, -0.6f, 0.7f),  Vector3(0.2f, -0.2f, -1.0f)};

    for (uint8_t i = 0; i < SPOTS; i++) {
      spot_direction[i] = directions[i].normalize();
      spot_color[i] = Color(i * 53, RainbowGradientPalette);
    }
  }

  void draw(float dt) {
    setMotionBlur(80);
    time += dt;

    const float cycle = 12.0f;
    const float half_cycle = cycle * 0.5f;
    float cycle_pos = fmodf(time, cycle);
    float ramp = cycle_pos < half_cycle ? cycle_pos / half_cycle
                                        : 1.0f - ((cycle_pos - half_cycle) /
                                                  half_cycle);
    ramp = smooth(ramp);

    const float speed = 10.0f + 520.0f * ramp;
    angle += dt * speed;

    Vector3 axis = Vector3(0.25f, 1.0f, 0.45f).normalize();
    Quaternion rotation(angle, axis);

    const float radius = 7.1f;
    const float shell = 0.85f;

    for (uint8_t x = 0; x < Display::width; x++) {
      for (uint8_t y = 0; y < Display::height; y++) {
        for (uint8_t z = 0; z < Display::depth; z++) {
          Vector3 p = Vector3(x - CX, y - CY, z - CZ);
          float d = p.magnitude();
          if (fabsf(d - radius) > shell) continue;

          Vector3 surface = d > 0.001f ? p / d : Vector3::Y;
          Color color(0, 6, 18);
          uint8_t brightness = 45;

          for (uint8_t i = 0; i < SPOTS; i++) {
            Vector3 spot_center = rotation.rotate(spot_direction[i]);
            float alignment = surface.dot(spot_center);
            if (alignment > 0.80f) {
              uint8_t spot_brightness =
                  70 + (uint8_t)(185.0f * (alignment - 0.80f) / 0.20f);
              Color spot = spot_color[i].scaled(spot_brightness);
              color.maximize(spot);
              brightness = 255;
            }
          }

          if (d < radius) brightness = brightness * 2 / 3;
          voxel(x, y, z, color.scale(brightness));
        }
      }
    }
  }
};

#endif
