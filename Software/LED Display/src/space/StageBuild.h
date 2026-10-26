#ifndef STAGE_BUILD_H
#define STAGE_BUILD_H

#include "YMCA.h"
#include "StageBuildTiming.h"

class StageBuild : public Animation {
 private:
  float age = 0;
  float readyAge = 0;
  float elapsed = 0;
  uint8_t hue = 0;

 public:
  void init() override {
    age = readyAge = elapsed = 0;
    hue = random(256);
    state = state_t::RUNNING;
    setMotionBlur(0);
  }

  void draw(float dt) override {
    elapsed += fmaxf(0.0f, dt);
    if (elapsed >= StageBuildTiming::READY)
      elapsed = StageBuildTiming::READY + fmodf(elapsed - StageBuildTiming::READY, 8.0f);
    drawAt(elapsed);
  }

  void drawAt(float seconds) {
    using namespace StageBuildTiming;
    age = fminf(READY, fmaxf(0.0f, seconds));
    readyAge = fmodf(fmaxf(0.0f, seconds - FLOOR_END), 8.0f);
    const float pulse = age < FLOOR_END ? 1 :
        0.72f + 0.28f * cosf(readyAge * YMCAPose::TAU * 2);
    const int colorFlow = static_cast<int>(readyAge * 96);

    for (uint8_t index = 0; index < 12; ++index) {
      const float head = edgeHead(index, age);
      for (uint8_t position = 0; position < 16; ++position) {
        if (head < position) continue;
        const bool tip = head < 16 && head - position < 1.5f;
        const uint8_t edgeHue = static_cast<uint8_t>(hue + index * 21 + colorFlow);
        const Color color = tip ? Color::WHITE :
            Color(edgeHue, RainbowGradientPalette).scaled(175 * pulse);
        const auto point = edgeAt(index, position);
        const auto wide = thicker(index, point);
        voxel(point.x, point.y, point.z, color);
        voxel(wide.x, wide.y, wide.z, color);
      }
    }
    for (uint8_t x = 0; x < 16; ++x)
      for (uint8_t z = 0; z < 16; ++z) {
        const float light = floorLight(x, z, age);
        if (light <= 0) continue;
        const uint8_t floorHue = static_cast<uint8_t>(hue + x * 5 + z * 5 + colorFlow);
        voxel(x, 0, z, Color(floorHue, RainbowGradientPalette).scaled(100 * light * pulse));
      }
    if (age > RISE_START) YMCA::drawFigure(figure(age));
  }
};
#endif
