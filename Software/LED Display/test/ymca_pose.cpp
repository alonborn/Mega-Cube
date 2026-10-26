#include "space/YMCAPose.h"
#include <assert.h>
#include <stdio.h>

static float distance(const YMCAPose::Point& a, const YMCAPose::Point& b) {
  const float x = a.x - b.x, y = a.y - b.y, z = a.z - b.z;
  return sqrtf(x * x + y * y + z * z);
}

int main() {
  using namespace YMCAPose;
  const Mode modes[] = {Mode::LETTERS, Mode::SIDE_STEPS, Mode::MARCH,
                         Mode::DISCO, Mode::CLAP, Mode::PERIMETER, Mode::HIP_SWAY};
  LetterCycles cycles;
  assert(!cycles.active());
  cycles.trigger();
  int lettersSeen = 0, previousLetter = -1;
  for (int frame = 0; frame < 7510; ++frame) {
    const int current = letterIndex(frame * 0.001f);
    if (current != previousLetter) ++lettersSeen;
    previousLetter = current;
  }
  assert(lettersSeen == 8);
  assert(letterColorIndex(-0.001f) == -1);
  assert(letterColorIndex(LETTER_DURATION) == -1);
  for (int index = 0; index < LETTER_COUNT; ++index) {
    const float start = LETTER_STARTS[index], end = LETTER_STARTS[index + 1];
    assert(letterColorIndex(start) == index);
    assert(letterColorIndex(end - 0.0001f) == (index == 3 ? -1 : index));
    for (int frame = 0; frame < 100; ++frame) {
      const float time = start + (end - start) * frame / 100;
      const bool gap = time >= LETTER_COLOR_BREAK_START && time < LETTER_STARTS[4];
      assert(letterColorIndex(time) == (gap ? -1 : index));
    }
  }
  assert(fabsf(LETTER_COLOR_BREAK_START - 2.12f) < 0.0001f);
  assert(fabsf(LETTER_COLOR_BREAK_START - LETTER_STARTS[3] - 0.5f) < 0.0001f);
  assert(fabsf(LETTER_COLOR_BREAK_SECONDS - 1.62f) < 0.0001f);
  assert(letterColorIndex(LETTER_COLOR_BREAK_START - 0.0001f) == 3);
  assert(letterColorIndex(LETTER_COLOR_BREAK_START) == -1);
  assert(letterColorIndex(LETTER_STARTS[4] - 0.0001f) == -1);
  assert(letterColorIndex(LETTER_STARTS[4]) == 4);
  cycles.advance(7.509f);
  assert(cycles.active());
  cycles.advance(0.002f);
  assert(!cycles.active());
  cycles.trigger();
  assert(cycles.active());
  cycles.reset();
  assert(!cycles.active());
  for (Mode mode : modes) {
    for (int frame = 0; frame <= 16000; ++frame) {
      const float time = frame / 16000.0f * loopSeconds(mode);
      const Pose pose = at(mode, time), next = at(mode, time + 0.00001f);
      for (int joint = 0; joint < JOINTS; ++joint) {
        const auto& p = pose.joint[joint];
        assert(isfinite(p.x) && isfinite(p.y) && isfinite(p.z));
        assert(p.x >= -7.5f && p.x <= 7.5f);
        assert(p.y >= 0 && p.y <= 15);
        assert(p.z >= -7.5f && p.z <= 7.5f);
        assert(distance(p, next.joint[joint]) < 0.003f);
      }
      assert(fminf(pose.joint[LEFT_FOOT].y, pose.joint[RIGHT_FOOT].y) < 0.0001f);
      assert(pose.joint[HEAD].y + 1.5f < 15.0f);
      assert(pose.joint[HEAD].x + 2.0f <= 7.5f);
      assert(pose.joint[HEAD].x - 1.0f >= -7.5f);
      assert(fabsf(pose.joint[HEAD].z) + 0.5f <= 7.5f);
      const Pose loop = at(mode, time + loopSeconds(mode));
      for (int joint = 0; joint < JOINTS; ++joint)
        assert(distance(pose.joint[joint], loop.joint[joint]) < 0.0002f);
    }
  }
  const Pose y = letter(0), m = letter(1), c = letter(2), a = letter(3);
  assert(y.joint[LEFT_HAND].x < -3 && y.joint[RIGHT_HAND].x > 3);
  assert(fabsf(m.joint[LEFT_ELBOW].x) > fabsf(a.joint[LEFT_ELBOW].x) + 1);
  assert(c.joint[LEFT_HAND].x > 1 && c.joint[RIGHT_HAND].x > 1);
  assert(distance(a.joint[LEFT_HAND], a.joint[RIGHT_HAND]) < 0.8f);
  assert(at(Mode::MARCH, MOVEMENT_SECONDS / 4.0f).joint[RIGHT_FOOT].y > 2.0f);
  assert(fabsf(loopSeconds(Mode::LETTERS) - 7.51f) < 0.0001f);
  const float cues[] = {0.0f, 0.92f, 1.32f, 1.62f, 3.74f, 4.67f, 5.13f, 5.39f};
  for (int index = 0; index < 8; ++index) {
    const Pose actual = at(Mode::LETTERS, cues[index]);
    const Pose target = letter(index);
    assert(fabsf(actual.joint[LEFT_HAND].x - target.joint[LEFT_HAND].x - 0.5f) < 0.0001f);
    assert(fabsf(actual.joint[RIGHT_HAND].x - target.joint[RIGHT_HAND].x - 0.5f) < 0.0001f);
  }
  assert(distance(at(Mode::CLAP, 0).joint[LEFT_HAND], at(Mode::CLAP, 0).joint[RIGHT_HAND]) < 0.8f);
  assert(fabsf(CLAP_SECONDS - 0.94f) < 0.0001f);
  assert(MOVEMENT_SECONDS == 0.9f);
  assert(fabsf(loopSeconds(Mode::CLAP) - 7.52f) < 0.0001f);
  const Pose openClap = at(Mode::CLAP, CLAP_SECONDS / 2.0f);
  assert(distance(openClap.joint[LEFT_HAND], openClap.joint[RIGHT_HAND]) > 6.0f);
  for (int cycle = 0; cycle < 8; ++cycle) {
    const Pose clap = at(Mode::CLAP, cycle * CLAP_SECONDS);
    assert(distance(clap.joint[LEFT_HAND], clap.joint[RIGHT_HAND]) < 0.8f);
  }
  const Mode walkingModes[] = {Mode::SIDE_STEPS, Mode::MARCH, Mode::PERIMETER};
  const Pose hipRight = at(Mode::HIP_SWAY, MOVEMENT_SECONDS / 4);
  const Pose hipLeft = at(Mode::HIP_SWAY, MOVEMENT_SECONDS * 0.75f);
  assert(hipRight.joint[HIP].x - hipLeft.joint[HIP].x > 2.1f);
  for (int frame = 0; frame < 1000; ++frame) {
    const auto pose = at(Mode::HIP_SWAY, frame * 0.01f);
    for (int side = 0; side < 2; ++side) {
      const float sign = side == 0 ? -1.0f : 1.0f;
      assert(fabsf(pose.joint[LEFT_HAND + side].x - pose.joint[HIP].x - sign * 1.1f) < 0.0001f);
      assert(fabsf(pose.joint[LEFT_HAND + side].y - pose.joint[HIP].y - 0.5f) < 0.0001f);
      assert(pose.joint[LEFT_FOOT + side].y == 0);
      assert(sign * pose.joint[LEFT_ELBOW + side].x > 1.4f);
    }
  }
  for (Mode mode : walkingModes) {
    const Joint first = mode == Mode::SIDE_STEPS ? LEFT_FOOT : RIGHT_FOOT;
    const Joint second = mode == Mode::SIDE_STEPS ? RIGHT_FOOT : LEFT_FOOT;
    assert(at(mode, MOVEMENT_SECONDS / 4.0f).joint[first].y > 0.5f);
    assert(at(mode, MOVEMENT_SECONDS * 0.75f).joint[second].y > 0.5f);
  }
  MarchTravel travel, other;
  travel.reset(12345);
  other.reset(67890);
  Path previous = {}, first = {};
  for (int frame = 0; frame < 60000; ++frame) {
    const float time = frame * 0.01f;
    const Path path = travel.advance(0.01f);
    if (frame == 719) first = path;
    if (frame == 1439) assert(hypotf(path.x - first.x, path.z - first.z) > 0.1f);
    assert(hypotf(path.x - previous.x, path.z - previous.z) <= 0.0136f);
    assert(fabsf(path.x) <= 3.8f && fabsf(path.z) <= 3.8f);
    const Pose pose = at(Mode::MARCH, time, &path);
    const Pose stationary = at(Mode::MARCH, time);
    for (int joint = 0; joint < JOINTS; ++joint) {
      assert(pose.joint[joint].y == stationary.joint[joint].y);
      assert(fabsf(pose.joint[joint].x) <= 7.5f);
      assert(fabsf(pose.joint[joint].z) <= 7.5f);
    }
    previous = path;
  }
  const Path alternative = other.advance(600.0f);
  assert(hypotf(previous.x - alternative.x, previous.z - alternative.z) > 0.1f);
  for (int step = 0; step < 1000; ++step) {
    const float beat = step / 1000.0f * 32.0f;
    const Path p = perimeter(beat), next = perimeter(beat + 0.001f);
    assert(fmaxf(fabsf(p.x), fabsf(p.z)) > 3.8f);
    assert(fabsf(p.x) <= 4.601f && fabsf(p.z) <= 4.601f);
    const float dx = next.x - p.x, dz = next.z - p.z;
    const float heading = (dx * sinf(p.yaw) + dz * cosf(p.yaw)) / sqrtf(dx * dx + dz * dz);
    assert(heading > 0.995f);
  }
  puts("YMCA: 0.9-second movement, 0.94-second claps, measured letters, cube bounds and continuous loops OK");
}
