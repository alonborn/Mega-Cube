#ifndef JUMPING_MAN_H
#define JUMPING_MAN_H

#include "WalkingMan.h"
#include "JumpingManPose.h"

class JumpingMan : public Animation {
 private:
  float cycle = 0.0f;

 public:
  void init() override {
    state = state_t::RUNNING;
    cycle = 0.0f;
    setMotionBlur(0);
  }

  void draw(float dt) override {
    cycle = fmodf(cycle + fmaxf(0.0f, dt) / 2.8f, 1.0f);
    WalkingMan::drawPose(JumpingManPose::at(cycle));
  }
};

#endif
