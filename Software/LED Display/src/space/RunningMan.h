#ifndef RUNNING_MAN_H
#define RUNNING_MAN_H

#include "WalkingMan.h"
#include "RunningManPose.h"

class RunningMan : public Animation {
 private:
  float cycle = 0.0f;

 public:
  void init() override {
    state = state_t::RUNNING;
    cycle = 0.0f;
    setMotionBlur(0);
  }

  void draw(float dt) override {
    cycle = fmodf(cycle + fmaxf(0.0f, dt) / 0.7f, 1.0f);
    WalkingMan::drawPose(RunningManPose::at(cycle));
  }
};

#endif
