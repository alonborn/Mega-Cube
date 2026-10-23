#ifndef MUSIC_PULSE_H
#define MUSIC_PULSE_H

#include "core/Graphics.h"
#include "core/LiveMusic.h"

inline void drawMusicPulse() {
  const float pulse = liveMusic().rhythmPulse;
  if (pulse < 0.015f) return;
  const Color color = Color(35, 90, 255).scaled(static_cast<uint8_t>(255.0f * pulse));
  for (uint8_t x = 0; x < Display::width; ++x)
    for (uint8_t y = 0; y < Display::height; ++y)
      for (uint8_t z = 0; z < Display::depth; ++z)
        Display::cube[Display::cubeBuffer][x][y][z].maximize(color);
}

#endif
