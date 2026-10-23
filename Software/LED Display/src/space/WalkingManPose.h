#ifndef WALKING_MAN_POSE_H
#define WALKING_MAN_POSE_H

#include <math.h>

namespace WalkingManPose {
constexpr float GAIT_PI = 3.14159265358979323846f;
constexpr float FLOOR = -4.15f;
constexpr float DISPLAY_SCALE = 4.0f / 3.0f;

struct Point {
  float x, y, z;
};

inline Point inCube(const Point& p) {
  return {0.5f + DISPLAY_SCALE * p.x,
           -7.5f + DISPLAY_SCALE * (p.y - FLOOR),
           0.5f + DISPLAY_SCALE * p.z};
}

struct Pose {
  Point hip, shoulder, neck, head;
  Point knees[2], feet[2], shoulders[2], elbows[2], hands[2];
  float headLift, headTilt;
};

inline Pose at(float cycle) {
  cycle -= floorf(cycle);
  const float phase = cycle * 2.0f * GAIT_PI;
  const float spread = sinf(phase);
  Pose pose = {};
  pose.headLift = 0.5f * (1.0f - cosf(phase * 2.0f));
  pose.headTilt = 0.3f * spread;
  pose.hip = {0.0f, -1.10f, 0.0f};
  pose.shoulder = {0.12f, pose.hip.y + 2.25f, 0.0f};
  pose.neck = {0.12f, pose.shoulder.y + 0.65f, 0.0f};
  pose.head = {0.12f, pose.neck.y + 1.05f, 0.0f};

  for (int side = 0; side < 2; ++side) {
    const float direction = side == 0 ? -1.0f : 1.0f;
    const float z = side == 0 ? -0.55f : 0.55f;
    // Deliberate scissor motion: feet cross while hands spread, and vice versa.
    pose.feet[side] = {direction * 1.6f * spread, FLOOR, z};
    pose.knees[side] = {pose.feet[side].x * 0.5f,
                        (pose.hip.y + FLOOR) * 0.5f, z};
    const float angle = -direction * 0.95f * spread;
    const Point shoulder = {pose.shoulder.x + direction * 0.35f, pose.shoulder.y, z};
    pose.shoulders[side] = shoulder;
    pose.elbows[side] = {shoulder.x + 1.15f * sinf(angle),
                         shoulder.y - 1.15f * cosf(angle), z};
    pose.hands[side] = {pose.elbows[side].x + 1.05f * sinf(angle),
                        pose.elbows[side].y - 1.05f * cosf(angle), z};
  }
  return pose;
}
}  // namespace WalkingManPose

#endif
