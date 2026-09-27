#ifndef BLACKOUT_H
#define BLACKOUT_H

#include "Animation.h"

class Blackout : public Animation {
 public:
  void init() override {
    state = state_t::RUNNING;
    setMotionBlur(0);
  }

  void draw(float dt) override {
    (void)dt;
  }
};

#endif
