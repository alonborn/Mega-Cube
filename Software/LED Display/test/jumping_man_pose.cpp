#include "space/JumpingManPose.h"
#include <assert.h>
#include <stdio.h>

static float distance(const WalkingManPose::Point& a, const WalkingManPose::Point& b) {
  const float dx = a.x - b.x, dy = a.y - b.y, dz = a.z - b.z;
  return sqrtf(dx * dx + dy * dy + dz * dz);
}

int main() {
  using namespace JumpingManPose;
  using WalkingManPose::inCube;
  for (int frame = 0; frame <= 10000; ++frame) {
    const float cycle = frame / 10000.0f;
    const Pose pose = at(cycle);
    const Pose next = at(cycle + 0.00001f);
    for (int side = 0; side < 2; ++side) {
      assert(fabsf(distance(pose.hips[side], pose.knees[side]) - 1.65f) < 0.0001f);
      assert(fabsf(distance(pose.knees[side], pose.feet[side]) - 1.65f) < 0.0001f);
      const Point joints[] = {pose.hips[side], pose.knees[side], pose.feet[side],
                              pose.shoulders[side], pose.elbows[side], pose.hands[side]};
      for (const auto& joint : joints) {
        const Point p = inCube(joint);
        assert(p.y >= -7.5001f && p.y <= 7.5f);
        assert(fabsf(p.x) < 7.5f && fabsf(p.z) < 7.5f);
      }
      if (cycle <= 0.34f || cycle >= 0.78f)
        assert(fabsf(inCube(pose.feet[side]).y + 7.5f) < 0.0001f);
      assert(distance(pose.feet[side], next.feet[side]) < 0.001f);
    }
    const float headTop = floorf(inCube(pose.head).y + 7.5f) + 2.0f;
    assert(headTop < 16.0f);
    assert(distance(pose.hip, next.hip) < 0.001f);
    assert(distance(pose.hands[0], next.hands[0]) < 0.002f);
  }
  assert(at(0.28f).hip.y < at(0).hip.y - 0.8f);
  assert(inCube(at(0.56f).feet[0]).y > -4.4f);
  assert(at(0.56f).hands[0].y > at(0.56f).shoulder.y);
  assert(at(0.86f).hip.y < at(0).hip.y - 0.5f);
  assert(distance(at(0).head, at(1).head) < 0.0001f);
  puts("Jumping Man: crouch, takeoff, airborne feet, raised arms, landing, leg lengths and cube bounds OK");
}
