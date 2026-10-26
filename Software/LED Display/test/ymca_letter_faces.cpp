#include "space/YMCALetterFaces.h"
#include <assert.h>
#include <stdio.h>

int main() {
  using namespace YMCALetterFaces;
  const char letters[] = "YMCA";
  const uint8_t boundaries[] = {0, 3, 5, 8, 10, 13};
  assert(SIZE == 13 && STROKE == 3 && BACK_Z == 15);
  for (uint8_t step = 0; step < 8; ++step) {
    for (uint8_t row = 0; row < 5; ++row)
      for (uint8_t col = 0; col < 5; ++col)
        for (uint8_t v = boundaries[row]; v < boundaries[row + 1]; ++v)
          for (uint8_t u = boundaries[col]; u < boundaries[col + 1]; ++u)
            assert(lit(step, u, v) == ((GLYPHS[step % 4][row] & (1 << (4 - col))) != 0));
    int pixels = 0;
    bool seen[16][16][16] = {};
    for (uint8_t u = 0; u < 16; ++u)
      for (uint8_t v = 0; v < 16; ++v) {
        if (!lit(step, u, v)) continue;
        assert(u < SIZE && v < SIZE);
        ++pixels;
        const auto p = point(u, v);
        assert(p.x >= 2 && p.x <= 14 && p.y >= 2 && p.y <= 14);
        assert(p.z == 15);
        assert(!seen[p.x][p.y][p.z]);
        seen[p.x][p.y][p.z] = true;
        assert(p.x == 2 + u && p.y == 14 - v);
      }
    assert(pixels > 20 && pixels < SIZE * SIZE);
    // Reconstruct the front-facing view: the asymmetric C must not be reversed.
    for (uint8_t v = 0; v < SIZE; ++v)
      for (uint8_t u = 0; u < SIZE; ++u)
        assert(seen[2 + u][14 - v][15] == lit(step, u, v));
  }
  assert(lit(0, 6, 12) && !lit(0, 1, 12) && !lit(0, 11, 12));
  assert(lit(1, 6, 6) && !lit(1, 3, 6) && !lit(1, 8, 6));
  assert(lit(2, 1, 6) && !lit(2, 11, 6));
  assert(lit(3, 6, 6) && !lit(3, 6, 3));
  for (uint8_t step = 0; step < 4; ++step) {
    printf("%c (13x13):\n", letters[step]);
    for (uint8_t v = 0; v < SIZE; ++v) {
      for (uint8_t u = 0; u < SIZE; ++u) putchar(lit(step, u, v) ? '#' : '.');
      putchar('\n');
    }
  }
  puts("YMCA letters: compact 13x13, rear Z=15 only, no horizontal mirroring, no other faces OK");
}
