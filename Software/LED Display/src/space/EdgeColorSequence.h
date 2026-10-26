#ifndef EDGE_COLOR_SEQUENCE_H
#define EDGE_COLOR_SEQUENCE_H

#include "WallFlash.h"
#include "EdgeColorTiming.h"

class EdgeColorSequence : public Animation {
 private:
  EdgeColorTiming::Sequence sequence;
  uint8_t hues[EdgeColorTiming::COLORS] = {};

 public:
  explicit EdgeColorSequence(bool alternate = false) : sequence(alternate) {}
  void init() override {
    sequence.trigger();
    hues[0] = random(256);
    for (uint8_t index = 1; index < sequence.colorCount(); ++index)
      hues[index] = static_cast<uint8_t>(hues[index - 1] + random(48, 209));
    state = state_t::RUNNING;
  }

  void draw(float dt) override {
    if (sequence.active()) drawWallColor(hues[sequence.index()]);
    sequence.advance(dt);
    if (!sequence.active()) state = state_t::INACTIVE;
  }
};
#endif
