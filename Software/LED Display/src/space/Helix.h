#ifndef HELIX_H
#define HELIX_H

#include "Animation.h"

class Helix : public Animation {
private:
  float radius;
  float resolution;

  float angle;
  float angle_speed;
  float phase;
  float phase_speed;

  uint8_t bottom;
  uint8_t top;
  uint8_t thickness;
  uint8_t stage;
  uint8_t direction = 5;
  float holdAge = 0.0f;

  Vector3 orient(Vector3 point) {
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

  Timer timer_interval;

  static constexpr auto &settings = config.animation.helix;

public:
  void init() {
    state = state_t::RUNNING;
    timer_interval = settings.interval * 0.5f;
    phase = 0;
    bottom = 0;
    top = 0;
    thickness = 0;
    stage = 0;
    holdAge = 0.0f;
    direction = (direction + 1) % 6;
  }

  void draw(float dt) {
    phase_speed = settings.phase_speed;
    angle = settings.angle;
    angle_speed = settings.angle_speed;
    hue16_speed = settings.hue_speed * 255;
    radius = settings.radius;
    resolution = settings.resolution;
    setMotionBlur(stage >= 3 ? 95 : settings.motionBlur);
    uint8_t brightness = settings.brightness * getBrightness();

    phase += dt * phase_speed;
    angle += dt * angle_speed;
    hue16 += dt * hue16_speed;

    Quaternion q1 = Quaternion(180, Vector3(0, 1, 0));
    Quaternion q2 = Quaternion(angle, Vector3(1, 0, 0));

    // Resize the function to be big enough to have the rotated version fit
    // sqrt(3) * 7.5 * 2 => 26 is big enough but more resolution is better
    for (uint16_t y = bottom; y <= top; y++) {
      float xf = sinf(phase + mapf(y, 0, resolution, 0, 2 * PI));
      float zf = cosf(phase + mapf(y, 0, resolution, 0, 2 * PI));
      Vector3 p0 = Vector3(xf, 2 * (y / resolution) - 1, zf) * radius;
      Vector3 p1 = orient(q2.rotate(p0));
      Vector3 p2 = orient((q2 * q1).rotate(p0));
      Color c1 = Color((hue16 >> 8) + y * 2 + 000, RainbowGradientPalette);
      Color c2 = Color((hue16 >> 8) + y * 2 + 128, RainbowGradientPalette);
      radiate(p1, c1.scale(brightness), 1.0f + (float)thickness / 20.0f);
      radiate(p2, c2.scale(brightness), 1.0f + (float)thickness / 20.0f);
    }
    if (stage == 2) {
      holdAge += dt;
      if (holdAge >= 1.5f || state == state_t::ENDING) stage = 3;
    }

    if (timer_interval.update()) {
      int progress = 0;
      if (stage == progress++) top <= resolution ? top++ : stage++;
      if (stage == progress++)
        thickness <= settings.thickness ? thickness++ : stage++;
      if (stage == progress++) {
        // Hold timing is updated every frame above.
      }
      if (stage == progress++)
        thickness > 0 ? thickness-- : stage++;
      if (stage == progress++) bottom <= resolution ? bottom++ : stage++;
      if (stage == progress++) state = state_t::INACTIVE;
    }
  }
  void end() {
    if (state == state_t::RUNNING) {
      timer_interval = timer_interval.set_time() / 5;
      state = state_t::ENDING;
      time_reduction = true;
    } else if (state == state_t::ENDING && !time_reduction) {
      timer_interval = timer_interval.set_time() / 5;
      state = state_t::ENDING;
      time_reduction = true;
    }
  }
};
#endif