#ifndef RUNNING_MAN_POSE_H
#define RUNNING_MAN_POSE_H

#include "WalkingManPose.h"

namespace RunningManPose {
using WalkingManPose::Point;
using WalkingManPose::Pose;

inline Pose at(float cycle) {
  cycle -= floorf(cycle);
  const float phase = cycle * 2.0f * WalkingManPose::GAIT_PI;
  const float bounce = 0.14f * (1.0f + cosf(2.0f * phase - 1.76f * WalkingManPose::GAIT_PI));
  Pose pose = {};
  pose.hip = {-0.25f, -1.35f + bounce, 0.0f};
  pose.shoulder = {0.3f, pose.hip.y + 2.2f, 0.0f};
  pose.neck = {0.55f, pose.shoulder.y + 0.65f, 0.0f};
  pose.head = {0.65f, pose.neck.y + 1.05f, 0.0f};
  pose.headTilt = -0.12f + 0.05f * sinf(phase * 2.0f);

  for (int side = 0; side < 2; ++side) {
    float step = cycle + side * 0.5f;
    step -= floorf(step);
    const float z = side == 0 ? -0.65f : 0.65f;
    float stride, lift;
    if (step < 0.38f) {
      stride = 1.45f * cosf(WalkingManPose::GAIT_PI * step / 0.38f);
      lift = 0.0f;
    } else {
      const float swing = (step - 0.38f) / 0.62f;
      stride = -1.45f * cosf(WalkingManPose::GAIT_PI * swing);
      lift = 1.65f * sinf(WalkingManPose::GAIT_PI * swing);
    }
    pose.hips[side] = {pose.hip.x, pose.hip.y, z};
    pose.feet[side] = {pose.hip.x + stride, WalkingManPose::FLOOR + lift, z};
    const float dy = pose.feet[side].y - pose.hip.y;
    const float length = sqrtf(stride * stride + dy * dy);
    const float bend = sqrtf(fmaxf(0.0f, 1.75f * 1.75f - length * length * 0.25f));
    pose.knees[side] = {pose.hip.x + stride * 0.5f - dy * bend / length,
                        pose.hip.y + dy * 0.5f + stride * bend / length, z};

    // Bent elbows counter the leg swing; knees point forward for a strong run silhouette.
    const float arm = -0.85f * stride / 1.45f;
    const Point shoulder = {pose.shoulder.x, pose.shoulder.y, z};
    pose.shoulders[side] = shoulder;
    pose.elbows[side] = {shoulder.x + 1.1f * sinf(arm),
                         shoulder.y - 1.1f * cosf(arm), z};
    pose.hands[side] = {pose.elbows[side].x + sinf(arm + 1.3f),
                        pose.elbows[side].y - cosf(arm + 1.3f), z};
  }
  return pose;
}
}  // namespace RunningManPose

#endif
