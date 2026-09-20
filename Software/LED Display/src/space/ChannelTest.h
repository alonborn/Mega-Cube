#ifndef CHANNELTEST_H
#define CHANNELTEST_H

#include "Animation.h"

class ChannelTest : public Animation {
 private:
  static constexpr float CHANNEL_TIME = 7.0f;
  float elapsed = 0.0f;
  uint8_t channel = 0;

 public:
  void init() override {
    state = state_t::RUNNING;
    elapsed = 0.0f;
    channel = 0;
  }

  void draw(float dt) override {
    elapsed += dt;
    while (elapsed >= CHANNEL_TIME) {
      elapsed -= CHANNEL_TIME;
      channel = (channel + 1) & 0x1F;
    }

    Color color = channel == 0 ? Color::RED : Color::WHITE;
    Display::testChannel(channel, color.bits());
  }
};

#endif
