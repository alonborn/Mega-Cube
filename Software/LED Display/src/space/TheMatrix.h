#ifndef THEMATRIX_H
#define THEMATRIX_H

#include "Animation.h"

class TheMatrix : public Animation {
 private:
  static const uint8_t STREAMS = 42;

  struct Stream {
    uint8_t x;
    uint8_t z;
    float y;
    float speed;
    uint8_t length;
    uint8_t brightness;
  };

  Stream streams[STREAMS];

  void resetStream(uint8_t i, bool start_inside) {
    streams[i].x = random(0, Display::width);
    streams[i].z = random(0, Display::depth);
    streams[i].y = start_inside ? random(0, Display::height)
                                : Display::height + random(0, 16) * 0.25f;
    streams[i].speed = noise.nextRandom(5.5f, 15.0f);
    streams[i].length = random(4, 11);
    streams[i].brightness = random(120, 256);
  }

 public:
  void init() {
    state = state_t::RUNNING;
    timer_running = config.animation.plasma.runtime;
    for (uint8_t i = 0; i < STREAMS; i++) {
      resetStream(i, true);
    }
  }

  void draw(float dt) {
    setMotionBlur(190);

    if (timer_running.update()) {
      state = state_t::INACTIVE;
      return;
    }

    for (uint8_t i = 0; i < STREAMS; i++) {
      Stream &stream = streams[i];
      stream.y -= stream.speed * dt;

      if (stream.y < -stream.length) {
        resetStream(i, false);
      }

      for (uint8_t tail = 0; tail < stream.length; tail++) {
        int16_t y = (int16_t)(stream.y + tail + 0.5f);
        if (y < 0 || y >= Display::height) continue;

        uint8_t level = stream.brightness / (tail + 1);
        Color c;
        if (tail == 0) {
          c = Color(40, 255, 80).scale(level);
        } else {
          c = Color(0, 220, 35).scale(level);
        }
        voxel(stream.x, y, stream.z, c);
      }

      if (random(0, 28) == 0) {
        voxel(stream.x, random(0, Display::height), stream.z,
              Color(40, 255, 80));
      }
    }
  }
};

#endif
