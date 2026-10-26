#ifndef YMCA_LETTER_FACES_H
#define YMCA_LETTER_FACES_H

#include <stdint.h>

namespace YMCALetterFaces {
struct Point { uint8_t x, y, z; };
constexpr uint8_t STROKE = 3;
constexpr uint8_t SIZE = 13;
constexpr uint8_t BACK_Z = 15;
// Keep main strokes three LEDs wide; narrower diagonal/gap cells reduce the size.
constexpr uint8_t GLYPHS[4][5] = {
    {0x11, 0x0a, 0x04, 0x04, 0x04}, // Y
    {0x11, 0x1b, 0x15, 0x11, 0x11}, // M
    {0x1f, 0x10, 0x10, 0x10, 0x1f}, // C
    {0x0e, 0x11, 0x1f, 0x11, 0x11}, // A
};

inline bool lit(uint8_t step, uint8_t u, uint8_t v) {
  if (u >= SIZE || v >= SIZE) return false;
  const uint8_t col = u < 3 ? 0 : u < 5 ? 1 : u < 8 ? 2 : u < 10 ? 3 : 4;
  const uint8_t row = v < 3 ? 0 : v < 5 ? 1 : v < 8 ? 2 : v < 10 ? 3 : 4;
  return (GLYPHS[step % 4][row] & (1 << (4 - col))) != 0;
}

inline Point point(uint8_t u, uint8_t v) {
  // Keep the confirmed rear face and remove horizontal mirroring for front viewing.
  return {static_cast<uint8_t>(2 + u), static_cast<uint8_t>(14 - v), BACK_Z};
}
}  // namespace YMCALetterFaces
#endif
