#include "space/StageBuildTiming.h"
#include "space/WallFlashTiming.h"
#include <assert.h>
#include <stdio.h>

int main() {
  using namespace StageBuildTiming;
  bool seen[12] = {}, mask[16][16][16] = {};
  for (uint8_t edge = 0; edge < 12; ++edge) {
    assert(ORDER[edge] < 12 && !seen[ORDER[edge]]);
    seen[ORDER[edge]] = true;
    const float start = BLACK_SECONDS + edge * EDGE_SECONDS;
    assert(edgeHead(edge, start - 0.001f) < 0);
    assert(edgeHead(edge, start + EDGE_SECONDS) > 15.9f);
    const auto first = edgeAt(edge, 0), last = edgeAt(edge, 15);
    assert(abs(int(first.x) - last.x) + abs(int(first.y) - last.y) +
           abs(int(first.z) - last.z) == 15);
    for (uint8_t position = 0; position < 16; ++position) {
      const auto p = edgeAt(edge, position), q = thicker(edge, p);
      assert(p.x < 16 && p.y < 16 && p.z < 16);
      assert(q.x < 16 && q.y < 16 && q.z < 16);
      assert(abs(int(p.x) - q.x) + abs(int(p.y) - q.y) + abs(int(p.z) - q.z) == 1);
      assert(WallFlashTiming::surface(p.x, p.y, p.z));
      assert(WallFlashTiming::surface(q.x, q.y, q.z));
      mask[p.x][p.y][p.z] = mask[q.x][q.y][q.z] = true;
    }
  }
  for (uint8_t x = 0; x < 16; ++x)
    for (uint8_t z = 0; z < 16; ++z) {
      assert(floorLight(x, z, EDGES_END) == 0);
      assert(floorLight(x, z, FLOOR_END) == 1);
      float previous = 0;
      for (int frame = 0; frame <= 100; ++frame) {
        const float time = EDGES_END + FLOOR_SECONDS * frame / 100;
        const float light = floorLight(x, z, time);
        assert(light >= previous && light >= 0 && light <= 1);
        if (x < 15) assert(light >= floorLight(x + 1, z, time));
        assert(light == floorLight(z, x, time));
        previous = light;
      }
      mask[x][0][z] = true;
    }
  for (uint8_t x = 0; x < 16; ++x)
    for (uint8_t y = 0; y < 16; ++y)
      for (uint8_t z = 0; z < 16; ++z)
        assert(mask[x][y][z] == WallFlashTiming::surface(x, y, z));
  assert(rise(RISE_START) == 0 && rise(READY) == 1);
  const auto hidden = figure(RISE_START), complete = figure(READY);
  for (int joint = 0; joint < YMCAPose::JOINTS; ++joint) {
    assert(hidden.joint[joint].y < -0.5f);
    assert(complete.joint[joint].y >= 0 && complete.joint[joint].y <= 15);
    assert(hidden.joint[joint].x == complete.joint[joint].x);
    assert(hidden.joint[joint].z == complete.joint[joint].z);
  }
  assert(hidden.joint[YMCAPose::HEAD].y + 1.5f < -0.5f);
  assert(fabsf(complete.joint[YMCAPose::HIP].x - 0.5f) < 0.001f);
  assert(fabsf(complete.joint[YMCAPose::HIP].z - 0.5f) < 0.001f);
  assert(fabsf(READY - BASE_SECONDS - 3.5f) < 0.0001f);
  assert(fabsf(READY - 11.67f) < 0.0001f);
  puts("Stage build: 12 unique 2x1 edges, retained stage mask, diagonal floor wash, hidden-to-center rise OK");
}
