#ifndef YMCA_FULL_H
#define YMCA_FULL_H

#include "YMCAFullTimeline.h"
#include "StageBuild.h"
#include "EdgeColorTiming.h"

void sendAnimationEvent(const char* event);
float getAudioLeadSeconds();

class YMCAFull : public Animation {
 private:
  StageBuild stage;
  YMCAFullTimeline::Cursor cues;
  YMCADiscoFrame::Worms worms[2];
  YMCAPose::MarchTravel travel;
  YMCAPose::Mode dance = YMCAPose::Mode::SIDE_STEPS;
  YMCAPose::Pose lastPose = {}, transitionPose = {};
  uint32_t startedAt = 0;
  float audioLead = 0, lastAge = 0, danceStart = 0, danceUntil = 0;
  float letterStart = -100, lightStart = -100;
  uint8_t lightAnimation = 0, lightHues[EdgeColorTiming::COLORS] = {};
  uint8_t letterHue = 0;
  int lastLetter = -1;
  bool finished = false;

  void chooseDance(float age) {
    const YMCAPose::Mode modes[] = {YMCAPose::Mode::SIDE_STEPS, YMCAPose::Mode::MARCH,
        YMCAPose::Mode::DISCO, YMCAPose::Mode::CLAP, YMCAPose::Mode::PERIMETER,
        YMCAPose::Mode::HIP_SWAY};
    YMCAPose::Mode next;
    do { next = modes[random(sizeof(modes) / sizeof(modes[0]))]; } while (next == dance);
    dance = next;
    transitionPose = lastPose;
    danceStart = age;
    danceUntil = age + random(4, 8) * YMCAPose::MOVEMENT_SECONDS * YMCAFullTimeline::DANCE_DURATION_SCALE;
  }

  void cue(const YMCAFullTimeline::Cue& entry) {
    if (entry.animation == 40) {
      letterStart = entry.seconds;
      lastLetter = -1;
      danceUntil = letterStart + YMCAFullTimeline::LETTER_SECONDS;
    } else if (entry.animation == 46 || entry.animation == 47 || entry.animation == 49) {
      lightAnimation = entry.animation;
      lightStart = entry.seconds;
      lightHues[0] = random(256);
      for (uint8_t index = 1; index < EdgeColorTiming::COLORS; ++index)
        lightHues[index] = static_cast<uint8_t>(lightHues[index - 1] + random(48, 209));
    }
  }

  void drawLighting(float age) {
    age = YMCAFullTimeline::lightingTime(age, lightAnimation);
    if (age < lightStart) return;
    if (lightAnimation == 46) {
      if (age - lightStart < WallFlashTiming::DURATION) drawWallColor(lightHues[0]);
    } else if (lightAnimation == 47 || lightAnimation == 49) {
      EdgeColorTiming::Sequence sequence(lightAnimation == 49);
      sequence.trigger();
      sequence.advance(age - lightStart);
      if (sequence.active()) drawWallColor(lightHues[sequence.index()]);
    }
  }

 public:
  void init() override {
    state = state_t::RUNNING;
    startedAt = micros();
    audioLead = getAudioLeadSeconds();
    lastAge = 0;
    finished = false;
    cues.reset();
    letterStart = lightStart = -100;
    lightAnimation = 0;
    lastLetter = -1;
    dance = YMCAPose::Mode::SIDE_STEPS;
    danceStart = StageBuildTiming::READY;
    danceUntil = StageBuildTiming::READY + 3.6f * YMCAFullTimeline::DANCE_DURATION_SCALE;
    lastPose = transitionPose = StageBuildTiming::figure(StageBuildTiming::READY);
    stage.init();
    travel.reset(random(1, 2147483647));
    worms[0].reset(random(1, 2147483647));
    worms[1].reset(random(1, 2147483647), true);
    setMotionBlur(0);
    sendAnimationEvent("YMCA_START");
  }

  void draw(float) override {
    using namespace YMCAFullTimeline;
    // Absolute monotonic song time prevents accumulated frame-time drift.
    const float songAge = static_cast<uint32_t>(micros() - startedAt) / 1000000.0f - audioLead;
    if (songAge < 0 || finished) return;
    if (songAge >= TRACK_SECONDS) {
      finished = true;
      sendAnimationEvent("YMCA_STOP");
      return;  // Remain RUNNING and black, so the scheduler cannot restart the song.
    }
    const float age = fmaxf(0.0f, songAge + VISUAL_OFFSET_SECONDS);
    const float dt = fmaxf(0.0f, age - lastAge);
    lastAge = age;
    int index;
    while ((index = cues.nextDue(age)) >= 0) cue(CUES[index]);
    if (age < StageBuildTiming::READY) {
      stage.drawAt(age);
      return;
    }

    YMCA::drawFrame(fmodf(age, YMCADiscoFrame::PERIOD));
    for (auto& group : worms) {
      group.advance(dt);
      for (uint8_t worm = 0; worm < YMCADiscoFrame::Worms::COUNT; ++worm)
        for (uint8_t position = 0; position < 16; ++position) {
          const auto sample = group.sample(worm, position);
          if (sample.brightness)
            voxel(sample.x, sample.y, sample.z,
                  Color(sample.hue, RainbowGradientPalette).scaled(sample.brightness));
        }
    }
    const auto path = travel.advance(dt);
    const float letterAge = age - letterStart;
    const bool letters = letterAge >= 0 && letterAge < LETTER_SECONDS;
    if (letters) {
      lastPose = YMCAPose::at(YMCAPose::Mode::LETTERS, letterAge);
    } else {
      if (age >= danceUntil) chooseDance(age);
      const auto target = YMCAPose::at(dance, age - danceStart, &path);
      lastPose = YMCAPose::mix(transitionPose, target,
          YMCAPose::smooth((age - danceStart) / DANCE_TRANSITION_SECONDS));
    }
    YMCA::drawFigure(lastPose);
    drawLighting(age);
    const float letterLightAge = lightingTime(age) - letterStart;
    const int8_t letterColor = YMCAPose::letterColorIndex(letterLightAge);
    if (letterColor >= 0) {
      if (letterColor != lastLetter) {
        letterHue = static_cast<uint8_t>(letterHue + random(48, 209));
        lastLetter = letterColor;
      }
      drawWallColor(letterHue);
      YMCA::drawLetter(letterColor, age);
    }
    const uint8_t fade = static_cast<uint8_t>(255 * endFade(songAge));
    if (fade < 255)
      for (uint8_t x = 0; x < Display::width; ++x)
        for (uint8_t y = 0; y < Display::height; ++y)
          for (uint8_t z = 0; z < Display::depth; ++z)
            Display::cube[Display::cubeBuffer][x][y][z].scale(fade);
  }
};
#endif
