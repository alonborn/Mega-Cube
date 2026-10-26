#ifndef YMCA_POSE_H
#define YMCA_POSE_H

#include <math.h>
#include <stdint.h>

namespace YMCAPose {
constexpr float TAU = 6.28318530718f;
constexpr float MOVEMENT_SECONDS = 0.9f;
constexpr float CLAP_SECONDS = 0.94f;
// The gait uses two half-beats for a complete left/right cycle or clap.
constexpr float BEAT_SECONDS = MOVEMENT_SECONDS / 2.0f;
constexpr float LETTER_STARTS[] = {0.0f, 0.92f, 1.32f, 1.62f, 3.74f,
                                  4.67f, 5.13f, 5.39f, 7.51f};
constexpr uint8_t LETTER_COUNT = 8;
constexpr float LETTER_DURATION = LETTER_STARTS[LETTER_COUNT];
constexpr float FIRST_A_COLOR_SECONDS = 0.5f;
constexpr float LETTER_COLOR_BREAK_START = LETTER_STARTS[3] + FIRST_A_COLOR_SECONDS;
constexpr float LETTER_COLOR_BREAK_SECONDS = LETTER_STARTS[4] - LETTER_COLOR_BREAK_START;

enum class Mode : uint8_t { LETTERS, SIDE_STEPS, MARCH, DISCO, CLAP, PERIMETER, HIP_SWAY };

inline uint8_t letterIndex(float seconds) {
  seconds = fmodf(fmaxf(0.0f, seconds), LETTER_DURATION);
  uint8_t current = 0;
  while (current < LETTER_COUNT - 1 && seconds >= LETTER_STARTS[current + 1]) ++current;
  return current;
}

inline int8_t letterColorIndex(float seconds) {
  if (seconds < 0 || seconds >= LETTER_DURATION) return -1;
  if (seconds >= LETTER_COLOR_BREAK_START && seconds < LETTER_STARTS[4]) return -1;
  return letterIndex(seconds);
}

inline float loopSeconds(Mode mode) {
  if (mode == Mode::LETTERS) return LETTER_DURATION;
  if (mode == Mode::CLAP) return 8 * CLAP_SECONDS;
  return (mode == Mode::PERIMETER ? 32.0f : 16.0f) * BEAT_SECONDS;
}

class LetterCycles {
 private:
  float elapsed = LETTER_DURATION;

 public:
  void trigger() { elapsed = 0; }
  void reset() { elapsed = LETTER_DURATION; }
  void advance(float dt) {
    elapsed = fminf(LETTER_DURATION, elapsed + fmaxf(0.0f, dt));
  }
  bool active() const { return elapsed < LETTER_DURATION; }
};
enum Joint : uint8_t {
  HIP, NECK, HEAD, LEFT_SHOULDER, RIGHT_SHOULDER, LEFT_ELBOW, RIGHT_ELBOW,
  LEFT_HAND, RIGHT_HAND, LEFT_KNEE, RIGHT_KNEE, LEFT_FOOT, RIGHT_FOOT, JOINTS
};

struct Point { float x, y, z; };
struct Pose { Point joint[JOINTS]; };

struct Path { float x, z, yaw; };

inline Path perimeter(float beats) {
  const float angle = TAU * beats / 32.0f;
  const float c = cosf(angle), s = sinf(angle);
  const float divisor = sqrtf(sqrtf(c * c * c * c + s * s * s * s));
  // A rounded-square route, inset enough to fit the full dancer at the edges.
  return {4.6f * c / divisor, 4.6f * s / divisor,
           atan2f(-s * s * s, c * c * c)};
}

inline float smooth(float t) {
  t = fmaxf(0.0f, fminf(1.0f, t));
  return t * t * (3.0f - 2.0f * t);
}

class MarchTravel {
 private:
  uint32_t state = 1;
  Path start = {}, target = {}, current = {};
  float elapsed = 0, duration = 1;

  float coordinate() {
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    return (state & 65535) / 65535.0f * 7.6f - 3.8f;
  }

  void chooseTarget() {
    start = current;
    float dx, dz, distance;
    do {
      target.x = coordinate();
      target.z = coordinate();
      dx = target.x - start.x;
      dz = target.z - start.z;
      distance = sqrtf(dx * dx + dz * dz);
    } while (distance < 3.0f);
    const float heading = atan2f(dx, dz);
    target.yaw = start.yaw + remainderf(heading - start.yaw, TAU);
    // Smoothstep peaks at 1.5 times average speed: at most 1.35 LEDs/second.
    duration = distance / 0.9f;
    elapsed = 0;
  }

 public:
  void reset(uint32_t seed) {
    state = seed ? seed : 1;
    current = {};
    chooseTarget();
  }

  Path advance(float dt) {
    dt = fmaxf(0.0f, dt);
    while (dt >= duration - elapsed) {
      dt -= duration - elapsed;
      current = target;
      current.yaw = remainderf(current.yaw, TAU);
      chooseTarget();
    }
    elapsed += dt;
    const float t = smooth(elapsed / duration);
    current = {start.x + (target.x - start.x) * t,
               start.z + (target.z - start.z) * t,
               start.yaw + (target.yaw - start.yaw) * smooth(elapsed / duration * 2)};
    return current;
  }
};

inline Pose mix(const Pose& a, const Pose& b, float t) {
  Pose pose = {};
  for (int i = 0; i < JOINTS; ++i) {
    const auto& p = a.joint[i];
    const auto& q = b.joint[i];
    pose.joint[i] = {p.x + (q.x - p.x) * t,
                     p.y + (q.y - p.y) * t, p.z + (q.z - p.z) * t};
  }
  return pose;
}

inline void arm(Pose& pose, int side, float ex, float ey, float hx, float hy) {
  const float z = side == 0 ? -0.3f : 0.3f;
  pose.joint[LEFT_ELBOW + side] = {ex, ey, z};
  pose.joint[LEFT_HAND + side] = {hx, hy, z};
}

inline Pose standing() {
  Pose pose = {};
  pose.joint[HIP] = {0, 4.3f, 0};
  pose.joint[NECK] = {0, 9.4f, 0};
  pose.joint[HEAD] = {0, 11.5f, 0};
  for (int side = 0; side < 2; ++side) {
    const float sign = side == 0 ? -1.0f : 1.0f;
    pose.joint[LEFT_SHOULDER + side] = {sign * 0.8f, 8.2f, sign * 0.2f};
    pose.joint[LEFT_KNEE + side] = {sign * 0.65f, 2.1f, sign * 0.4f};
    pose.joint[LEFT_FOOT + side] = {sign * 1.05f, 0, sign * 0.5f};
    arm(pose, side, sign * 1.4f, 6.4f, sign * 1.7f, 4.5f);
  }
  return pose;
}

inline Pose letter(uint8_t index) {
  Pose pose = standing();
  switch (index % 4) {
    case 0:  // Y: long diagonals, well separated above the shoulders.
      arm(pose, 0, -2.1f, 10.7f, -3.5f, 13.6f);
      arm(pose, 1, 2.1f, 10.7f, 3.5f, 13.6f);
      break;
    case 1:  // M: wide bent elbows, hands drawn in towards the crown.
      arm(pose, 0, -3.3f, 11.0f, -0.65f, 13.1f);
      arm(pose, 1, 3.3f, 11.0f, 0.65f, 13.1f);
      break;
    case 2:  // C: upper and lower arms curve around the left, open to the right.
      arm(pose, 0, -2.8f, 11.1f, 1.5f, 13.5f);
      arm(pose, 1, -2.3f, 6.2f, 1.6f, 5.8f);
      break;
    default: // A: hands meet above the head, with a narrow triangular outline.
      arm(pose, 0, -1.9f, 11.4f, -0.2f, 14.1f);
      arm(pose, 1, 1.9f, 11.4f, 0.2f, 14.1f);
      break;
  }
  return pose;
}

inline void bounce(Pose& pose, float lift) {
  for (int joint = HIP; joint <= RIGHT_HAND; ++joint) pose.joint[joint].y += lift;
}

inline Pose at(Mode mode, float seconds, const Path* travel = nullptr) {
  seconds = fmodf(fmaxf(0.0f, seconds), loopSeconds(mode));
  const float beats = seconds / (mode == Mode::CLAP ? CLAP_SECONDS / 2 : BEAT_SECONDS);
  const float beatPhase = TAU * beats;
  Pose pose = standing();
  float rootX = 0, rootZ = 0, yaw = 0;

  switch (mode) {
    case Mode::LETTERS: {
      const uint8_t current = letterIndex(seconds);
      // Arrive at each measured cue after a short, continuous transition.
      const float transition = smooth((seconds - (LETTER_STARTS[current + 1] - 0.08f)) / 0.08f);
      pose = mix(letter(current), letter(current + 1), transition);
      bounce(pose, 0.09f * (1.0f - cosf(TAU * seconds / LETTER_DURATION * 16)));
      break;
    }
    case Mode::SIDE_STEPS: {
      const float step = sinf(beatPhase * 0.5f);
      rootX = 1.25f * sinf(beatPhase * 0.25f);
      for (int side = 0; side < 2; ++side) {
        const float sign = side == 0 ? -1.0f : 1.0f;
        const float lift = 0.55f * fmaxf(0.0f, -sign * step);
        pose.joint[LEFT_FOOT + side].x = sign * (1.1f + 0.6f * fmaxf(0.0f, sign * step));
        pose.joint[LEFT_FOOT + side].y = lift;
        pose.joint[LEFT_KNEE + side].x = pose.joint[LEFT_FOOT + side].x * 0.6f;
        pose.joint[LEFT_KNEE + side].y += lift * 0.6f;
        arm(pose, side, sign * 2.2f, 8.8f, sign * 3.4f, 10.2f + sign * 0.8f * step);
      }
      bounce(pose, 0.12f * (1.0f - cosf(beatPhase)));
      break;
    }
    case Mode::MARCH:
    case Mode::PERIMETER: {
      const bool around = mode == Mode::PERIMETER;
      if (around) {
        const Path path = perimeter(beats);
        rootX = path.x;
        rootZ = path.z;
        yaw = path.yaw;
      } else if (travel) {
        rootX = travel->x;
        rootZ = travel->z;
        yaw = travel->yaw;
      }
      for (int side = 0; side < 2; ++side) {
        const float sign = side == 0 ? -1.0f : 1.0f;
        const float swing = sign * sinf(beatPhase * 0.5f);
        const float lift = fmaxf(0.0f, swing);
        pose.joint[LEFT_FOOT + side].y = lift * (around ? 1.2f : 2.2f);
        pose.joint[LEFT_FOOT + side].z += lift * 0.8f;
        pose.joint[LEFT_KNEE + side].y = 2.1f + lift * (around ? 1.0f : 1.9f);
        pose.joint[LEFT_KNEE + side].z += lift * 0.6f;
        arm(pose, side, sign * (1.5f - 0.6f * swing), 6.8f - 0.7f * swing,
            sign * (1.5f - 0.6f * swing), 8.0f - 1.1f * swing);
      }
      bounce(pose, 0.12f * (1.0f - cosf(beatPhase)));
      break;
    }
    case Mode::DISCO: {
      Pose left = standing(), right = standing();
      arm(left, 0, -2.3f, 10.6f, -4.3f, 13.5f);
      arm(left, 1, 1.9f, 6.5f, 2.9f, 4.1f);
      arm(right, 0, -1.9f, 6.5f, -2.9f, 4.1f);
      arm(right, 1, 2.3f, 10.6f, 4.3f, 13.5f);
      const float bar = beats / 2.0f;
      const bool reverse = static_cast<int>(bar) % 2;
      const float progress = bar - floorf(bar);
      pose = mix(reverse ? right : left, reverse ? left : right, smooth((progress - 0.7f) / 0.3f));
      pose.joint[HIP].x = 0.55f * sinf(beatPhase * 0.5f);
      bounce(pose, 0.1f * (1.0f - cosf(beatPhase)));
      break;
    }
    case Mode::HIP_SWAY: {
      const float sway = sinf(beatPhase * 0.5f);
      pose.joint[HIP].x = 1.1f * sway;
      pose.joint[NECK].x = -0.2f * sway;
      pose.joint[HEAD].x = -0.15f * sway;
      for (int side = 0; side < 2; ++side) {
        const float sign = side == 0 ? -1.0f : 1.0f;
        pose.joint[LEFT_SHOULDER + side].x -= 0.25f * sway;
        pose.joint[LEFT_KNEE + side].x += 0.3f * sway;
        arm(pose, side, sign * 2.4f + 0.35f * sway, 6.5f,
            sign * 1.1f + pose.joint[HIP].x, 4.8f);
      }
      bounce(pose, 0.06f * (1.0f - cosf(beatPhase)));
      break;
    }
    case Mode::CLAP: {
      const float opening = 0.5f * (1.0f - cosf(beatPhase * 0.5f));
      for (int side = 0; side < 2; ++side) {
        const float sign = side == 0 ? -1.0f : 1.0f;
        arm(pose, side, sign * (1.4f + 0.8f * opening), 11.2f,
            sign * (0.15f + 3.4f * opening), 13.8f - opening * 0.7f);
      }
      bounce(pose, 0.08f * (1.0f - cosf(beatPhase)));
      break;
    }
  }

  const float c = cosf(yaw), s = sinf(yaw);
  for (auto& point : pose.joint) {
    const float x = point.x, z = point.z;
    point.x = 0.5f + rootX + c * x + s * z;
    point.z = 0.5f + rootZ - s * x + c * z;
  }
  return pose;
}
}  // namespace YMCAPose

#endif
