#include "space/WalkingManPose.h"
#include <assert.h>
#include <stdio.h>

static float distance(const WalkingManPose::Point& a, const WalkingManPose::Point& b) {
  const float dx = a.x - b.x, dy = a.y - b.y, dz = a.z - b.z;
  return sqrtf(dx * dx + dy * dy + dz * dz);
}

int main() {
  using namespace WalkingManPose;
  for (int frame = 0; frame <= 1000; ++frame) {
    const float phase = frame / 1000.0f;
    const Pose pose = at(phase);
    const Pose opposite = at(phase + 0.5f);
    assert(pose.headLift >= 0.0f && pose.headLift <= 1.0001f);
    assert(fabsf(pose.headTilt) <= 0.3001f);
    assert(fabsf(pose.headLift - opposite.headLift) < 0.0001f);
    assert(fabsf(pose.headTilt + opposite.headTilt) < 0.0001f);
    for (int side = 0; side < 2; ++side) {
      assert(fabsf(pose.feet[side].y - FLOOR) < 0.0001f);
      assert(fabsf(pose.knees[side].x - pose.feet[side].x * 0.5f) < 0.0001f);
      const float handSwing = pose.hands[side].x - pose.shoulders[side].x;
      assert(handSwing * pose.feet[side].x <= 0.0001f);
      const Point foot = inCube(pose.feet[side]);
      assert(foot.y >= -7.5001f && foot.y < -6.3f);
      assert(fabsf(foot.x) < 7.5f && fabsf(foot.z) < 7.5f);
      assert(fabsf(pose.feet[side].x - opposite.feet[1 - side].x) < 0.0001f);
      assert(fabsf(pose.feet[side].y - opposite.feet[1 - side].y) < 0.0001f);
    }
    assert(fminf(pose.feet[0].y, pose.feet[1].y) < FLOOR + 0.0001f);
    assert(fabsf(fminf(inCube(pose.feet[0]).y, inCube(pose.feet[1]).y) + 7.5f) < 0.0001f);
    const float height = pose.head.y + 0.9f - FLOOR;
    assert(height > 7.8f && height < 8.1f);
    assert(height * DISPLAY_SCALE > 10.4f && height * DISPLAY_SCALE < 10.8f);
    const Pose next = at(phase + 0.0001f);
    assert(distance(pose.feet[0], next.feet[0]) < 0.002f);
    assert(distance(pose.hands[0], next.hands[0]) < 0.002f);
    assert(fabsf(pose.headLift - next.headLift) < 0.001f);
    assert(fabsf(pose.headTilt - next.headTilt) < 0.001f);
  }
  assert(at(0.25f).feet[0].x < at(0.25f).feet[1].x);
  assert(at(0.25f).hands[0].x > at(0.25f).hands[1].x);
  assert(at(0.75f).feet[0].x > at(0.75f).feet[1].x);
  assert(at(0.75f).hands[0].x < at(0.75f).hands[1].x);
  assert(at(0.25f).headLift > 0.99f && at(0.25f).headTilt > 0.29f);
  assert(at(0.75f).headLift > 0.99f && at(0.75f).headTilt < -0.29f);
  puts("Walking Man: scissor motion, opposite arms, floor contact, head bounce/tilt and continuous loop OK");
}
