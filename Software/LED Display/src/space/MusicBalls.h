#ifndef MUSIC_BALLS_H
#define MUSIC_BALLS_H

#include "Animation.h"
#include "core/LiveMusic.h"
#include "MusicPulse.h"

class MusicBalls : public Animation {
 private:
  float phase = 0.0f;
  float tilt = 0.0f;

 public:
  void init() override {
    state = state_t::RUNNING;
    phase = tilt = 0.0f;
    liveMusic().reset();
    setMotionBlur(0);
  }

  void draw(float dt) override {
    LiveMusicInput& music = liveMusic();
    music.advance(dt, millis());
    phase = fmodf(phase + dt * (1.0f + 18.0f * music.level + 5.0f * music.beat),
                  6.2831853f);
    tilt += dt * (0.2f + 1.4f * music.bands[4]);
    const float elevation = 0.65f * sinf(tilt);
    const Vector3 center(5.7f * cosf(phase) * cosf(elevation),
                         5.7f * sinf(elevation),
                         5.7f * sinf(phase) * cosf(elevation));
    const float radius = 1.15f + music.bands[0] * 0.6f + music.beat * 0.6f;
    const float light = fminf(1.0f, music.level * 1.25f + music.beat * 0.3f);
    if (light < 0.008f) {
      drawMusicPulse();
      return;
    }
    for (uint8_t x = 0; x < 16; ++x) {
      for (uint8_t y = 0; y < 16; ++y) {
        for (uint8_t z = 0; z < 16; ++z) {
          const Vector3 point(x - CX, y - CY, z - CZ);
          const Vector3 a = point - center;
          const Vector3 b = point + center;
          const float da = a.dot(a);
          const float db = b.dot(b);
          const float distance = fminf(da, db);
          if (distance > radius * radius) continue;
          const float shading = light * (0.45f + 0.55f * (1.0f - distance / (radius * radius)));
          const Color color = da < db ? Color(15, 110, 255) : Color(255, 65, 8);
          voxel(x, y, z, color.scaled(static_cast<uint8_t>(255.0f * shading)));
        }
      }
    }
    drawMusicPulse();
  }
};

#endif
