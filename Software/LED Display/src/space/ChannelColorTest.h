#ifndef CHANNELCOLORTEST_H
#define CHANNELCOLORTEST_H

#include "Animation.h"

class ChannelColorTest : public Animation {
 private:
  static constexpr float COLOR_SPEED = 160.0f;
  float color_phase = 0.0f;

 public:
  void init() override {
    state = state_t::RUNNING;
    color_phase = 0.0f;
  }

  void draw(float dt) override {
    color_phase += dt * COLOR_SPEED;

    Color color(static_cast<uint8_t>(color_phase), RainbowGradientPalette);
    Display::testAllChannels(color.bits());
  }
};

#endif
