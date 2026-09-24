#ifndef EYEOFSAURON_H
#define EYEOFSAURON_H

#include "Animation.h"

class EyeOfSauron : public Animation {
 private:
  static constexpr float DURATION = 30.0f;

  enum class GazeMode : uint8_t { SCAN, DROP, HOLD, TRACK, SNAP };

  float age = 0.0f;
  float phaseAge = 0.0f;
  float phaseDuration = 4.5f;
  float azimuth = 0.0f;
  float elevation = -0.2f;
  float phaseStartAzimuth = 0.0f;
  float phaseStartElevation = -0.2f;
  float phaseTargetAzimuth = 0.0f;
  int8_t scanDirection = 1;
  GazeMode gazeMode = GazeMode::SCAN;

  float smoothStep(float value) {
    value = constrain(value, 0.0f, 1.0f);
    return value * value * (3.0f - 2.0f * value);
  }

  void beginPhase(GazeMode mode, float duration) {
    gazeMode = mode;
    phaseAge = 0.0f;
    phaseDuration = duration;
    phaseStartAzimuth = azimuth;
    phaseStartElevation = elevation;
  }

  void updateGaze(float dt) {
    phaseAge += dt;

    switch (gazeMode) {
      case GazeMode::SCAN:
        azimuth += scanDirection * dt *
                   (2.5f + 0.65f * sinf(age * 3.1f));
        elevation = -0.2f + 0.3f * sinf(age * 1.7f);
        if (phaseAge >= phaseDuration) beginPhase(GazeMode::DROP, 0.65f);
        break;

      case GazeMode::DROP: {
        const float amount = smoothStep(phaseAge / phaseDuration);
        elevation = phaseStartElevation + (-3.0f - phaseStartElevation) * amount;
        if (phaseAge >= phaseDuration)
          beginPhase(GazeMode::HOLD, noise.nextRandom(1.0f, 2.0f));
        break;
      }

      case GazeMode::HOLD:
        elevation = -3.0f;
        if (phaseAge >= phaseDuration)
          beginPhase(GazeMode::TRACK, noise.nextRandom(1.5f, 2.5f));
        break;

      case GazeMode::TRACK:
        elevation = -3.0f;
        azimuth += scanDirection * dt * 0.32f;
        if (phaseAge >= phaseDuration) {
          beginPhase(GazeMode::SNAP, 0.55f);
          phaseTargetAzimuth = azimuth + scanDirection * PI;
        }
        break;

      case GazeMode::SNAP: {
        const float amount = smoothStep(phaseAge / phaseDuration);
        azimuth = phaseStartAzimuth +
                  (phaseTargetAzimuth - phaseStartAzimuth) * amount;
        elevation = phaseStartElevation + (-0.2f - phaseStartElevation) * amount;
        if (phaseAge >= phaseDuration) {
          scanDirection = -scanDirection;
          beginPhase(GazeMode::SCAN, noise.nextRandom(3.5f, 6.0f));
        }
        break;
      }
    }
  }

  void drawTower() {
    const uint8_t SIDES = 8;
    const Vector3 apex(0, 2.7f, 0);

    for (uint8_t i = 0; i < SIDES; ++i) {
      const float angle = i * TWO_PI / SIDES;
      Vector3 base(cosf(angle) * 1.65f, -7.8f, sinf(angle) * 1.65f);
      Vector3 shoulder(cosf(angle) * 0.5f, 2.3f,
                       sinf(angle) * 0.5f);
      line(base, shoulder, Color(32, 2, 2));
      line(shoulder, apex, Color(75, 5, 0));

      if ((i & 1) == 0) {
        Vector3 ember = base * 0.35f + shoulder * 0.65f;
        radiate4(ember, Color(100, 8, 0), 0.45f);
      }
    }

    line(Vector3(-1.65f, -7.8f, 0), Vector3(1.65f, -7.8f, 0),
         Color(38, 2, 2));
    line(Vector3(0, -7.8f, -1.65f), Vector3(0, -7.8f, 1.65f),
         Color(38, 2, 2));
  }

  void drawEye(const Vector3 &eye, const Vector3 &look) {
    const Vector3 right(look.z, 0, -look.x);
    const Vector3 up(0, 1, 0);
    const uint8_t SEGMENTS = 12;

    Vector3 previousTop = eye - right * 3.0f;
    Vector3 previousBottom = previousTop;
    for (uint8_t i = 1; i <= SEGMENTS; ++i) {
      const float t = i / static_cast<float>(SEGMENTS);
      const float across = -3.0f + t * 6.0f;
      const float height = 1.35f * sinf(t * PI);
      Vector3 top = eye + right * across + up * height;
      Vector3 bottom = eye + right * across - up * height;
      line(previousTop, top, Color(255, 55, 0));
      line(previousBottom, bottom, Color(190, 8, 0));
      previousTop = top;
      previousBottom = bottom;
    }

    radiate4(eye, Color(255, 150, 15), 1.15f);
    line(eye - up * 1.0f, eye + up * 1.0f, Color(255, 245, 150));
  }

  void drawSearchlight(const Vector3 &eye, const Vector3 &look) {
    const Vector3 target = eye + look * 13.0f;
    const Vector3 right(look.z, 0, -look.x);
    const Vector3 up(0, 1, 0);

    line(eye, target, Color(230, 0, 0));
    for (uint8_t i = 0; i < 8; ++i) {
      const float angle = i * TWO_PI / 8.0f;
      Vector3 edge = target + right * (cosf(angle) * 1.25f) +
                     up * (sinf(angle) * 1.25f);
      line(eye, edge, Color(85, 0, 0));
    }

    radiate4(target, Color(255, 12, 0), 1.8f);
    radiate4(target, Color(255, 95, 10), 0.75f);
  }

 public:
  void init() override {
    state = state_t::RUNNING;
    age = 0.0f;
    phaseAge = 0.0f;
    phaseDuration = 4.5f;
    azimuth = 0.0f;
    elevation = -0.2f;
    scanDirection = 1;
    gazeMode = GazeMode::SCAN;
    setMotionBlur(95);
  }

  void draw(float dt) override {
    age += dt;
    updateGaze(dt);
    Vector3 look(sinf(azimuth), elevation, cosf(azimuth));
    look.normalize();

    const Vector3 eye(0, 4.7f, 0);
    drawTower();
    drawSearchlight(eye, look);
    drawEye(eye, look);

    if (age >= DURATION) state = state_t::INACTIVE;
  }
};

#endif
