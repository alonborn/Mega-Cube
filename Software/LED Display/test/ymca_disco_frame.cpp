#include "space/YMCADiscoFrame.h"
#include <assert.h>
#include <stdio.h>

int main() {
  using namespace YMCADiscoFrame;
  bool seen[16][16][16] = {};
  bool hues[256] = {};
  int unique = 0, colors = 0;
  for (uint8_t edge = 0; edge < 12; ++edge)
    for (uint8_t pixel = 0; pixel < 16; ++pixel) {
      const auto sample = at(edge, pixel, 0.0f);
      assert(sample.x < 16 && sample.y < 16 && sample.z < 16);
      const int boundaries = (sample.x == 0 || sample.x == 15) +
                             (sample.y == 0 || sample.y == 15) +
                             (sample.z == 0 || sample.z == 15);
      assert(boundaries >= 2);
      if (!seen[sample.x][sample.y][sample.z]) ++unique;
      seen[sample.x][sample.y][sample.z] = true;
      if (!hues[sample.hue]) ++colors;
      hues[sample.hue] = true;
      const auto loop = at(edge, pixel, PERIOD);
      assert(loop.hue == sample.hue && loop.brightness == sample.brightness);
      assert(at(edge, pixel, 1.0f).hue != sample.hue);
      for (int frame = 0; frame <= 800; ++frame) {
        const auto pulse = at(edge, pixel, frame * 0.01f);
        assert(pulse.brightness >= 114 && pulse.brightness <= 165);
      }
    }
  assert(unique == 176 && colors > 100);
  assert(!seen[8][8][8] && !seen[8][0][8] && !seen[8][15][8]);
  for (int x = 0; x <= 15; x += 15)
    for (int y = 0; y <= 15; y += 15)
      for (int z = 0; z <= 15; z += 15) assert(seen[x][y][z]);
  bool floorHues[256] = {};
  int tileColors = 0;
  for (uint8_t x = 0; x < 16; ++x)
    for (uint8_t z = 0; z < 16; ++z) {
      const auto tile = floorAt(x, z, 0.0f);
      assert(tile.x == x && tile.y == 0 && tile.z == z);
      assert(tile.brightness >= 28 && tile.brightness <= 105);
      if (!floorHues[tile.hue]) ++tileColors;
      floorHues[tile.hue] = true;
      const auto loop = floorAt(x, z, PERIOD);
      assert(tile.hue == loop.hue && tile.brightness == loop.brightness);
      assert(tile.hue != floorAt(x, z, 1.0f).hue);
      if (x < 13) assert(floorAt(x, z, 0.0625f).hue == floorAt(x + 3, z, 0.0f).hue);
    }
  assert(tileColors > 100);
  for (uint8_t edge = 8; edge < 12; ++edge)
    for (uint8_t y = 0; y < 15; ++y) {
      const auto down = at(edge, y, 0.03125f), above = at(edge, y + 1, 0.0f);
      assert(down.hue == above.hue);
      assert(down.brightness == above.brightness);
    }
  assert(static_cast<uint8_t>(floorAt(5, 5, 0.25f).hue - floorAt(5, 5, 0).hue) == 96);
  for (int frame = 0; frame < 200; ++frame) {
    const float time = frame * 0.01f;
    const auto a = floorAt(5, 5, time), b = floorAt(5, 5, time + 2.0f);
    assert(a.hue == b.hue && a.brightness == b.brightness);
  }
  Worms worms, repeat, different;
  worms.reset(12345);
  repeat.reset(12345);
  different.reset(54321);
  bool heights[16] = {}, faces[4] = {};
  int simultaneous = 0, distinct = 0;
  for (int frame = 0; frame < 12000; ++frame) {
    worms.advance(0.01f);
    repeat.advance(0.01f);
    different.advance(0.01f);
    int visible = 0;
    for (uint8_t index = 0; index < Worms::COUNT; ++index) {
      int pixels = 0, height = -1;
      for (uint8_t position = 0; position < 16; ++position) {
        const auto p = worms.sample(index, position);
        const auto q = repeat.sample(index, position);
        const auto r = different.sample(index, position);
        assert(p.x == q.x && p.y == q.y && p.z == q.z &&
               p.hue == q.hue && p.brightness == q.brightness);
        if (p.hue != r.hue || p.brightness != r.brightness) ++distinct;
        if (!p.brightness) continue;
        assert(p.x < 16 && p.z < 16 && p.y >= 2 && p.y <= 13);
        assert(p.x == 0 || p.x == 15 || p.z == 0 || p.z == 15);
        assert(p.brightness <= 180);
        if (height >= 0) assert(p.y == height);
        height = p.y;
        heights[p.y] = true;
        if (p.z == 0) faces[0] = true;
        if (p.x == 15) faces[1] = true;
        if (p.z == 15) faces[2] = true;
        if (p.x == 0) faces[3] = true;
        ++pixels;
      }
      assert(pixels <= 7);
      if (pixels) ++visible;
    }
    if (visible > simultaneous) simultaneous = visible;
  }
  assert(simultaneous >= 3 && distinct > 1000);
  Worms vertical, horizontal;
  vertical.reset(31415, true);
  horizontal.reset(31415);
  bool wallLanes[4][16] = {}, levels[16] = {};
  int upward = 0, downward = 0;
  for (int frame = 0; frame < 60000; ++frame) {
    vertical.advance(0.01f);
    horizontal.advance(0.01f);
    for (uint8_t index = 0; index < Worms::COUNT; ++index) {
      int firstY = -1;
      int firstX = -1, firstZ = -1;
      for (uint8_t position = 0; position < 16; ++position) {
        const auto p = vertical.sample(index, position);
        const auto h = horizontal.sample(index, position);
        assert(p.brightness == h.brightness && p.hue == h.hue);
        if (!p.brightness) continue;
        assert(p.x == 0 || p.x == 15 || p.z == 0 || p.z == 15);
        assert(p.y < 16);
        if (p.z == 0) wallLanes[0][p.x] = true;
        if (p.x == 15) wallLanes[1][p.z] = true;
        if (p.z == 15) wallLanes[2][15 - p.x] = true;
        if (p.x == 0) wallLanes[3][15 - p.z] = true;
        levels[p.y] = true;
        if (firstY >= 0) {
          assert(p.x == firstX && p.z == firstZ);
          if (p.y > firstY) ++upward;
          else ++downward;
        }
        firstY = p.y;
        firstX = p.x;
        firstZ = p.z;
      }
    }
  }
  for (const auto& wall : wallLanes)
    for (bool lane : wall) assert(lane);
  for (bool level : levels) assert(level);
  assert(upward > 100 && downward > 100);
  for (int height = 2; height <= 13; ++height) assert(heights[height]);
  for (bool face : faces) assert(face);
  // Subdividing a frame must not change the spawn schedule or travel.
  Worms whole, split;
  whole.reset(42);
  split.reset(42);
  whole.advance(0.5f);
  for (int step = 0; step < 50; ++step) split.advance(0.01f);
  for (uint8_t index = 0; index < Worms::COUNT; ++index)
    for (uint8_t position = 0; position < 16; ++position) {
      const auto a = whole.sample(index, position), b = split.sample(index, position);
      assert(a.x == b.x && a.y == b.y && a.z == b.z);
      assert(abs(int(a.brightness) - int(b.brightness)) <= 1);
    }
  puts("YMCA disco: 4x color speed, descending gradients, flowing floor and seamless loops OK");
  puts("Disco worms: horizontal walls, random heights/colors, overlapping trails and frame-rate independence OK");
}
