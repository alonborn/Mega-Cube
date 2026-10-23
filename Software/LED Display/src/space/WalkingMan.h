#ifndef WALKING_MAN_H
#define WALKING_MAN_H

#include "Animation.h"
#include "WalkingManPose.h"

class WalkingMan : public Animation {
 private:
  float cycle = 0.0f;

  static Vector3 point(const WalkingManPose::Point& p) {
    const auto placed = WalkingManPose::inCube(p);
    return Vector3(placed.x, placed.y, placed.z);
  }

  static void bone(const WalkingManPose::Point& a,
                   const WalkingManPose::Point& b, const Color& color) {
    line(point(a), point(b), color);
  }

 public:
  void init() override {
    state = state_t::RUNNING;
    cycle = 0.0f;
    setMotionBlur(0);
  }

  void draw(float dt) override {
    cycle = fmodf(cycle + fmaxf(0.0f, dt) * 0.8f, 1.0f);
    const auto pose = WalkingManPose::at(cycle);
    const Color white(255, 255, 255);

    for (int side = 0; side < 2; ++side) {
      const Color limb = side == 0 ? Color(255, 100, 12) : Color(15, 220, 255);
      const WalkingManPose::Point hip = {pose.hip.x, pose.hip.y, pose.feet[side].z};
      bone(pose.hip, hip, limb);
      bone(hip, pose.knees[side], limb);
      bone(pose.knees[side], pose.feet[side], limb);
      bone(pose.shoulder, pose.shoulders[side], limb);
      bone(pose.shoulders[side], pose.elbows[side], limb);
      bone(pose.elbows[side], pose.hands[side], limb);
    }
    bone(pose.hip, pose.shoulder, white);
    const Vector3 head = point(pose.head);
    const Vector3 center(floorf(head.x + CX) + 0.5f - CX,
                          floorf(head.y + CY) + 0.5f - CY + pose.headLift,
                          floorf(head.z + CZ + 0.5f) - CZ);
    const float c = cosf(pose.headTilt);
    const float s = sinf(pose.headTilt);
    line(point(pose.shoulder), center + Vector3(1.5f * s, -1.5f * c, 0), white);
    // Eight LEDs form a hollow round outline: two per side, no corner pixels.
    for (int x = 0; x < 4; ++x)
      for (int y = 0; y < 4; ++y)
        if ((x == 0 || x == 3) != (y == 0 || y == 3)) {
          const float dx = x - 1.5f;
          const float dy = y - 1.5f;
          voxel(center + Vector3(c * dx - s * dy, s * dx + c * dy, 0), white);
        }
  }
};

#endif
