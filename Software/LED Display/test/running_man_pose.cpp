#include "space/RunningManPose.h"
#include <assert.h>
#include <stdio.h>

static float distance(const WalkingManPose::Point& a, const WalkingManPose::Point& b) {
  const float dx = a.x - b.x, dy = a.y - b.y, dz = a.z - b.z;
  return sqrtf(dx * dx + dy * dy + dz * dz);
}

int main() {
  using namespace RunningManPose;
  using WalkingManPose::inCube;
  bool hasFlight = false, hasContact = false, hasRaisedKnee = false;
  for (int frame = 0; frame <= 10000; ++frame) {
    const float cycle = frame / 10000.0f;
    const Pose pose = at(cycle), next = at(cycle + 0.00001f), opposite = at(cycle + 0.5f);
    hasFlight |= pose.feet[0].y > WalkingManPose::FLOOR + 0.1f &&
                 pose.feet[1].y > WalkingManPose::FLOOR + 0.1f;
    hasContact |= fabsf(inCube(pose.feet[0]).y + 7.5f) < 0.0001f;
    assert(pose.shoulder.x > pose.hip.x + 0.5f);
    for (int side = 0; side < 2; ++side) {
      assert(fabsf(distance(pose.hips[side], pose.knees[side]) - 1.75f) < 0.0001f);
      assert(fabsf(distance(pose.knees[side], pose.feet[side]) - 1.75f) < 0.0001f);
      assert(fabsf(pose.feet[side].x - opposite.feet[1 - side].x) < 0.0001f);
      assert(fabsf(pose.feet[side].y - opposite.feet[1 - side].y) < 0.0001f);
      const float legSwing = pose.feet[side].x - pose.hip.x;
      const float armSwing = pose.elbows[side].x - pose.shoulder.x;
      assert(legSwing * armSwing <= 0.0001f);
      const Point joints[] = {pose.hips[side], pose.knees[side], pose.feet[side],
                              pose.shoulders[side], pose.elbows[side], pose.hands[side]};
      for (const auto& joint : joints) {
        const Point p = inCube(joint);
        assert(p.y >= -7.5001f && p.y <= 7.5f);
        assert(fabsf(p.x) < 7.5f && fabsf(p.z) < 7.5f);
      }
      hasRaisedKnee |= pose.knees[side].y > pose.hip.y - 0.5f;
      assert(distance(pose.feet[side], next.feet[side]) < 0.001f);
      assert(distance(pose.hands[side], next.hands[side]) < 0.001f);
    }
    assert(inCube(pose.head).y + 2.5f < 7.5f);
  }
  assert(hasFlight && hasContact && hasRaisedKnee);
  assert(distance(at(0).head, at(1).head) < 0.0001f);
  puts("Running Man: high knees, fixed leg lengths, opposing arms, flight/contact, bounds and loop continuity OK");
}
