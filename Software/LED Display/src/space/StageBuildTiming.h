#ifndef STAGE_BUILD_TIMING_H
#define STAGE_BUILD_TIMING_H

#include "YMCADiscoFrame.h"
#include "YMCAPose.h"

namespace StageBuildTiming {
constexpr float BASE_SECONDS = 0.25f + 12 * 0.36f + 1.2f + 0.4f + 2.0f;
constexpr float TIME_SCALE = (BASE_SECONDS + 3.5f) / BASE_SECONDS;
constexpr float BLACK_SECONDS = 0.25f * TIME_SCALE;
constexpr float EDGE_SECONDS = 0.36f * TIME_SCALE;
constexpr float EDGES_END = BLACK_SECONDS + 12 * EDGE_SECONDS;
constexpr float FLOOR_SECONDS = 1.2f * TIME_SCALE;
constexpr float FLOOR_END = EDGES_END + FLOOR_SECONDS;
constexpr float RISE_START = FLOOR_END + 0.4f * TIME_SCALE;
constexpr float RISE_SECONDS = 2.0f * TIME_SCALE;
constexpr float READY = RISE_START + RISE_SECONDS;
constexpr uint8_t ORDER[] = {0, 1, 2, 3, 8, 4, 5, 6, 7, 11, 10, 9};

inline float edgeHead(uint8_t index, float seconds) {
  return (seconds - BLACK_SECONDS - index * EDGE_SECONDS) / EDGE_SECONDS * 16;
}

inline YMCADiscoFrame::Sample edgeAt(uint8_t index, uint8_t position) {
  const uint8_t edge = ORDER[index];
  const bool reverse = edge == 9 || edge == 11;
  return YMCADiscoFrame::at(edge, reverse ? 15 - position : position, 0);
}

inline YMCADiscoFrame::Sample thicker(uint8_t index, YMCADiscoFrame::Sample sample) {
  const uint8_t edge = ORDER[index];
  if (edge >= 8 || edge % 2 == 1) sample.x += sample.x == 0 ? 1 : -1;
  else sample.z += sample.z == 0 ? 1 : -1;
  return sample;
}

inline float floorLight(uint8_t x, uint8_t z, float seconds) {
  if (seconds <= EDGES_END) return 0;
  if (seconds >= FLOOR_END) return 1;
  const float front = (seconds - EDGES_END) / FLOOR_SECONDS * 34 - 2;
  return YMCAPose::smooth((front - x - z) / 2);
}

inline float rise(float seconds) {
  return YMCAPose::smooth((seconds - RISE_START) / RISE_SECONDS);
}

inline YMCAPose::Pose figure(float seconds) {
  auto pose = YMCAPose::at(YMCAPose::Mode::SIDE_STEPS, 0);
  const float offset = -16 * (1 - rise(seconds));
  for (auto& point : pose.joint) point.y += offset;
  return pose;
}
}  // namespace StageBuildTiming
#endif
