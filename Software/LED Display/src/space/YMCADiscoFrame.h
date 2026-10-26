#ifndef YMCA_DISCO_FRAME_H
#define YMCA_DISCO_FRAME_H

#include <math.h>
#include <stdint.h>

namespace YMCADiscoFrame {
constexpr float PERIOD = 8.0f;
constexpr float COLOR_SPEED = 4.0f;

struct Sample {
  uint8_t x, y, z, hue, brightness;
};

inline Sample floorAt(uint8_t x, uint8_t z, float seconds) {
  const float age = fmodf(fmaxf(0.0f, seconds) * COLOR_SPEED, PERIOD);
  Sample sample = {x, 0, z, 0, 0};
  sample.hue = static_cast<uint8_t>(static_cast<int>(age * 96.0f) + x * 8 + z * 5);
  const bool grout = x % 4 == 0 || z % 4 == 0;
  sample.brightness = grout ? 28 : static_cast<uint8_t>(
      85.0f + 20.0f * sinf(age * 3.14159265f + x * 0.39269908f + z * 0.26179939f));
  return sample;
}

inline Sample at(uint8_t edge, uint8_t position, float seconds) {
  Sample sample = {};
  if (edge < 8) {
    sample.y = edge < 4 ? 0 : 15;
    switch (edge % 4) {
      case 0: sample.x = position; sample.z = 0; break;
      case 1: sample.x = 15; sample.z = position; break;
      case 2: sample.x = 15 - position; sample.z = 15; break;
      default: sample.x = 0; sample.z = 15 - position; break;
    }
  } else {
    sample.y = position;
    sample.x = edge == 9 || edge == 10 ? 15 : 0;
    sample.z = edge >= 10 ? 15 : 0;
  }
  const float age = fmodf(fmaxf(0.0f, seconds) * COLOR_SPEED, PERIOD);
  const float flow = age * 96.0f;
  sample.hue = static_cast<uint8_t>(static_cast<int>(flow) + edge * 19 +
                                     position * (edge >= 8 ? 12 : 5));
  // Gentle pulses keep the colorful frame below the dancer's arm brightness.
  const float wave = edge >= 8 ? (position + age * 8.0f) * 0.78539816f
                               : age * 12.5663706f;
  sample.brightness = static_cast<uint8_t>(140.0f + 25.0f * sinf(wave + edge * 0.6f));
  return sample;
}

class Worms {
 public:
  static constexpr uint8_t COUNT = 8;

 private:
  struct Trail {
    uint8_t edge, height, length, hue;
    bool reverse;
    float head, speed;
  };
  Trail trails[COUNT] = {};
  uint32_t state = 1;
  float untilSpawn = 0;
  bool vertical = false;

  uint32_t next() {
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    return state;
  }

  void move(float dt) {
    for (auto& trail : trails) trail.head += trail.speed * dt;
  }

  void spawn() {
    for (auto& trail : trails) {
      if (trail.head <= 15 + trail.length) continue;
      trail.edge = next() % 4;
      trail.height = vertical ? next() % 16 : 2 + next() % 12;
      trail.length = 3 + next() % 4;
      trail.hue = next() % 256;
      trail.reverse = next() % 2;
      trail.head = 0;
      trail.speed = 20 + next() % 13;
      break;
    }
    untilSpawn = 0.14f + (next() % 161) * 0.001f;
  }

 public:
  void reset(uint32_t seed, bool upright = false) {
    state = seed ? seed : 1;
    vertical = upright;
    for (auto& trail : trails) trail = {0, 2, 3, 0, false, 32, 0};
    untilSpawn = 0;
  }

  void advance(float dt) {
    dt = fmaxf(0.0f, dt);
    // Process spawn times independently of frame rate and choreography loops.
    while (dt >= untilSpawn) {
      move(untilSpawn);
      dt -= untilSpawn;
      spawn();
    }
    move(dt);
    untilSpawn -= dt;
  }

  Sample sample(uint8_t index, uint8_t position) const {
    if (index >= COUNT || position > 15) return {};
    const auto& trail = trails[index];
    const float behind = trail.head - position;
    if (behind < 0 || behind >= trail.length + 1) return {};
    const uint8_t along = trail.reverse ? 15 - position : position;
    // For vertical trails, height stores the fixed horizontal lane on the face.
    Sample result = at(trail.edge, vertical ? trail.height : along, 0);
    result.y = vertical ? along : trail.height;
    result.hue = static_cast<uint8_t>(trail.hue + static_cast<int>(behind * 10));
    result.brightness = static_cast<uint8_t>(180 * (1 - behind / (trail.length + 1)));
    return result;
  }
};
}  // namespace YMCADiscoFrame

#endif
