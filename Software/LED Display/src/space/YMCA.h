#ifndef YMCA_H
#define YMCA_H

#include "Animation.h"
#include "YMCAPose.h"
#include "YMCADiscoFrame.h"
#include "WallFlash.h"
#include "YMCALetterFaces.h"

class YMCA : public Animation {
 private:
  const YMCAPose::Mode mode;
  float age = 0.0f;
  float frameAge = 0.0f;
  YMCAPose::MarchTravel marchTravel;
  YMCADiscoFrame::Worms worms[2];
  int8_t flashedLetter = -1;
  uint8_t flashHue = 0;

 public:
  static void drawFrame(float seconds) {
    for (uint8_t x = 0; x < 16; ++x)
      for (uint8_t z = 0; z < 16; ++z) {
        const auto tile = YMCADiscoFrame::floorAt(x, z, seconds);
        voxel(tile.x, tile.y, tile.z,
              Color(tile.hue, RainbowGradientPalette).scaled(tile.brightness));
      }
    for (uint8_t edge = 0; edge < 12; ++edge)
      for (uint8_t position = 0; position < 16; ++position) {
        const auto sample = YMCADiscoFrame::at(edge, position, seconds);
        voxel(sample.x, sample.y, sample.z,
              Color(sample.hue, RainbowGradientPalette).scaled(sample.brightness));
      }
  }

 private:
  static Vector3 point(const YMCAPose::Point& p) { return Vector3(p.x, p.y - CY, p.z); }

  static void bone(const YMCAPose::Pose& pose, YMCAPose::Joint a,
                   YMCAPose::Joint b, const Color& color) {
    line(point(pose.joint[a]), point(pose.joint[b]), color);
  }

 public:
  explicit YMCA(YMCAPose::Mode selected) : mode(selected) {}

  void init() override {
    state = state_t::RUNNING;
    age = 0.0f;
    frameAge = 0.0f;
    if (mode == YMCAPose::Mode::MARCH) marchTravel.reset(random(1, 2147483647));
    worms[0].reset(random(1, 2147483647));
    worms[1].reset(random(1, 2147483647), true);
    flashedLetter = -1;
    setMotionBlur(0);
  }

  void draw(float dt) override {
    using namespace YMCAPose;
    age = fmodf(age + fmaxf(0.0f, dt), loopSeconds(mode));
    frameAge = fmodf(frameAge + fmaxf(0.0f, dt), YMCADiscoFrame::PERIOD);
    Path travel = {};
    if (mode == Mode::MARCH) travel = marchTravel.advance(dt);
    const Pose pose = at(mode, age, &travel);
    drawFrame(frameAge);
    for (auto& group : worms) {
      group.advance(dt);
      for (uint8_t index = 0; index < YMCADiscoFrame::Worms::COUNT; ++index)
        for (uint8_t position = 0; position < 16; ++position) {
          const auto sample = group.sample(index, position);
          if (sample.brightness)
            voxel(sample.x, sample.y, sample.z,
                  Color(sample.hue, RainbowGradientPalette).scaled(sample.brightness));
        }
    }
    drawFigure(pose);
    if (mode == Mode::LETTERS) {
      const uint8_t current = letterIndex(age);
      if (flashedLetter != current) {
        flashedLetter = current;
        flashHue = static_cast<uint8_t>(flashHue + random(48, 209));
      }
      if (letterColorIndex(age) >= 0) {
        drawWallColor(flashHue);
        drawLetter(current, age);
      }
    }
  }

  static void drawLetter(uint8_t step, float seconds) {
    const int flow = static_cast<int>(fmodf(fmaxf(0.0f, seconds), 8.0f) * 96);
    for (uint8_t u = 0; u < YMCALetterFaces::SIZE; ++u)
      for (uint8_t v = 0; v < YMCALetterFaces::SIZE; ++v) {
        if (!YMCALetterFaces::lit(step, u, v)) continue;
        const uint8_t hue = static_cast<uint8_t>(flow + u * 7 + v * 5 + step * 53);
        const Color color = Color(hue, RainbowGradientPalette).scaled(235);
        const auto p = YMCALetterFaces::point(u, v);
        voxel(p.x, p.y, p.z, color);
      }
  }

  static void drawFigure(const YMCAPose::Pose& pose) {
    using namespace YMCAPose;
    const Color body(180, 170, 150);
    const Color arms(15, 240, 255);
    const Color white(255, 255, 255);

    bone(pose, HIP, LEFT_KNEE, Color(40, 95, 230));
    bone(pose, LEFT_KNEE, LEFT_FOOT, Color(40, 95, 230));
    bone(pose, HIP, RIGHT_KNEE, Color(35, 145, 255));
    bone(pose, RIGHT_KNEE, RIGHT_FOOT, Color(35, 145, 255));
    bone(pose, HIP, NECK, body);
    bone(pose, LEFT_SHOULDER, RIGHT_SHOULDER, body);

    const Vector3 head = point(pose.joint[HEAD]) + Vector3(0.5f, 0, 0);
    for (int x = 0; x < 4; ++x)
      for (int y = 0; y < 4; ++y)
        if ((x == 0 || x == 3) != (y == 0 || y == 3)) {
          const Vector3 rim(x - 1.5f, y - 1.5f, 0);
          voxel(head + rim + Vector3(0, 0, -0.5f), white.scaled(95));
          voxel(head + rim + Vector3(0, 0, 0.5f), white.scaled(180));
        }

    // Draw the bright choreography last, keeping it clear against head and torso.
    for (int side = 0; side < 2; ++side) {
      const auto shoulder = static_cast<Joint>(LEFT_SHOULDER + side);
      const auto elbow = static_cast<Joint>(LEFT_ELBOW + side);
      const auto hand = static_cast<Joint>(LEFT_HAND + side);
      bone(pose, shoulder, elbow, arms);
      bone(pose, elbow, hand, arms);
      voxel(point(pose.joint[hand]), white);
    }
  }
};

#endif
