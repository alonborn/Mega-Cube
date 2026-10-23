#ifndef MUSIC_EQUALIZER_H
#define MUSIC_EQUALIZER_H

#include "Animation.h"
#include "core/LiveMusic.h"
#include "MusicPulse.h"

class MusicEqualizer : public Animation {
 private:
  uint8_t history[16][8] = {};
  float elapsed = 0.0f;

 public:
  void init() override {
    state = state_t::RUNNING;
    memset(history, 0, sizeof(history));
    elapsed = 0.0f;
    liveMusic().reset();
    setMotionBlur(0);
  }

  void draw(float dt) override {
    LiveMusicInput& music = liveMusic();
    music.advance(dt, millis());
    elapsed += fminf(dt, 0.1f);
    if (elapsed >= 0.05f) {
      elapsed = fmodf(elapsed, 0.05f);
      for (int z = 15; z > 0; --z) memcpy(history[z], history[z - 1], 8);
      for (uint8_t i = 0; i < 8; ++i)
        history[0][i] = static_cast<uint8_t>(music.bands[i] * 255.0f);
    }
    const Color colors[8] = {Color(20, 30, 255), Color(0, 130, 255),
                             Color(0, 220, 190), Color(20, 220, 45),
                             Color(200, 230, 0), Color(255, 110, 0),
                             Color(255, 25, 35), Color(200, 20, 180)};
    for (uint8_t z = 0; z < 16; ++z) {
      for (uint8_t band = 0; band < 8; ++band) {
        const float value = z == 0 ? music.bands[band] : history[z][band] / 255.0f;
        const uint8_t height = static_cast<uint8_t>(value * 16.0f + 0.5f);
        const uint8_t brightness = static_cast<uint8_t>((65.0f + value * 190.0f) * (1.0f - z * 0.045f));
        for (uint8_t y = 0; y < height && y < 16; ++y) {
          Color color = colors[band].scaled(brightness);
          if (z == 0 && y + 1 == height && music.beat > 0.3f)
            color.maximize(Color::WHITE.scaled(static_cast<uint8_t>(music.beat * 180)));
          voxel(band * 2, y, 15 - z, color);
          voxel(band * 2 + 1, y, 15 - z, color);
        }
      }
    }
    drawMusicPulse();
  }
};

#endif
