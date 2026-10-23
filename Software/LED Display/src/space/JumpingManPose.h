#ifndef JUMPING_MAN_POSE_H
#define JUMPING_MAN_POSE_H

#include "WalkingManPose.h"

namespace JumpingManPose {
using WalkingManPose::Point;
using WalkingManPose::Pose;

inline float smooth(float t) {
  t = fmaxf(0.0f, fminf(t, 1.0f));
  return t * t * (3.0f - 2.0f * t);
}

inline Pose at(float cycle) {
  cycle -= floorf(cycle);
  float crouch = 0.0f;
  float lift = 0.0f;
  float arms = 0.0f;
  if (cycle < 0.28f) {
    const float prepare = smooth((cycle - 0.07f) / 0.21f);
    crouch = 0.85f * prepare;
    arms = -0.7f * prepare;
  } else if (cycle < 0.34f) {
    const float push = smooth((cycle - 0.28f) / 0.06f);
    crouch = 0.85f * (1.0f - push);
    arms = -0.7f + 3.3f * push;
  } else if (cycle < 0.78f) {
    const float flight = (cycle - 0.34f) / 0.44f;
    lift = 2.4f * 4.0f * flight * (1.0f - flight);
    crouch = 0.2f * sinf(WalkingManPose::GAIT_PI * flight);
    arms = 2.6f - 2.3f * smooth((flight - 0.55f) / 0.45f);
  } else if (cycle < 0.94f) {
    const float landing = (cycle - 0.78f) / 0.16f;
    crouch = 0.6f * sinf(WalkingManPose::GAIT_PI * landing);
    arms = 0.3f * (1.0f - smooth(landing));
  }

  Pose pose = {};
  pose.hip = {0.0f, -1.10f + lift - crouch, 0.0f};
  pose.shoulder = {0.12f, pose.hip.y + 2.25f, crouch * 0.3f};
  pose.neck = {0.12f, pose.shoulder.y + 0.65f, pose.shoulder.z};
  pose.head = {0.12f, pose.neck.y + 1.05f, pose.shoulder.z};
  pose.headTilt = -0.1f * crouch;

  for (int side = 0; side < 2; ++side) {
    const float direction = side == 0 ? -1.0f : 1.0f;
    pose.hips[side] = {direction * 0.15f, pose.hip.y, direction * 0.55f};
    pose.feet[side] = {direction * 0.8f, WalkingManPose::FLOOR + lift, direction * 0.7f};
    const auto& hip = pose.hips[side];
    const auto& foot = pose.feet[side];
    const float dx = foot.x - hip.x, dy = foot.y - hip.y, dz = foot.z - hip.z;
    const float square = dx * dx + dy * dy + dz * dz;
    // Bend each equal-length leg towards the front, not sideways into the other leg.
    const float nx = -dx * dz / square;
    const float ny = -dy * dz / square;
    const float nz = 1.0f - dz * dz / square;
    const float bend = sqrtf(fmaxf(0.0f, 1.65f * 1.65f - square * 0.25f)) /
                       sqrtf(nx * nx + ny * ny + nz * nz);
    pose.knees[side] = {hip.x + dx * 0.5f + nx * bend,
                        hip.y + dy * 0.5f + ny * bend,
                        hip.z + dz * 0.5f + nz * bend};

    const Point shoulder = {pose.shoulder.x + direction * 0.55f,
                             pose.shoulder.y, direction * 0.55f + pose.shoulder.z};
    pose.shoulders[side] = shoulder;
    pose.elbows[side] = {shoulder.x + direction * 0.15f,
                         shoulder.y - 1.15f * cosf(arms),
                         shoulder.z + 1.15f * sinf(arms)};
    pose.hands[side] = {shoulder.x + direction * 0.3f,
                        pose.elbows[side].y - 1.05f * cosf(arms),
                        pose.elbows[side].z + 1.05f * sinf(arms)};
  }
  return pose;
}
}  // namespace JumpingManPose

#endif
