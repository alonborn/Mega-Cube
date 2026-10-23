#include "AnimationCatalog.h"
#include <assert.h>
#include <stdio.h>

static AnimationCatalog::Result chunk(AnimationCatalog& catalog, uint8_t id,
                                     uint8_t offset, const char* name, uint8_t count,
                                     bool& changed) {
  uint8_t packet[20] = {'A', 'C', 1, id, offset, static_cast<uint8_t>(strlen(name))};
  memcpy(packet + 6, name + offset, count);
  uint8_t resultId = 0;
  const auto result = catalog.accept(packet, count + 6, resultId, changed);
  if (result == AnimationCatalog::COMPLETE) assert(resultId == id);
  return result;
}

int main() {
  AnimationCatalog catalog;
  bool changed = false;
  assert(strcmp(catalog.name(39), "MUSIC EQ") == 0);
  assert(catalog.name(40)[0] == '\0');
  assert(catalog.adjacent(39, 1) == 0);
  const char* name = "ABCDEFGHIJKLMNOPQRSTUVWXYZ123456";
  assert(chunk(catalog, 40, 0, name, 14, changed) == AnimationCatalog::PARTIAL);
  assert(catalog.name(40)[0] == '\0');
  assert(chunk(catalog, 40, 28, name, 4, changed) == AnimationCatalog::INVALID);
  assert(chunk(catalog, 40, 14, name, 14, changed) == AnimationCatalog::PARTIAL);
  assert(chunk(catalog, 40, 28, name, 4, changed) == AnimationCatalog::COMPLETE && changed);
  assert(strcmp(catalog.name(40), name) == 0);
  assert(catalog.adjacent(39, 1) == 40);
  assert(catalog.adjacent(40, 1) == 0);
  assert(chunk(catalog, 40, 0, "NEW NAME", 8, changed) == AnimationCatalog::COMPLETE && changed);
  assert(chunk(catalog, 40, 0, "NEW NAME", 8, changed) == AnimationCatalog::COMPLETE && !changed);
  assert(chunk(catalog, 39, 0, "OVERRIDE", 8, changed) == AnimationCatalog::COMPLETE && !changed);
  assert(strcmp(catalog.name(39), "MUSIC EQ") == 0);
  assert(chunk(catalog, 41, 0, "   ", 3, changed) == AnimationCatalog::INVALID);
  assert(!catalog.restore(41, "bad\nname"));
  assert(!catalog.restore(0, "OVERRIDE"));
  assert(catalog.restore(255, "LAST ANIMATION"));
  assert(catalog.adjacent(0, -1) == 255);
  assert(catalog.adjacent(255, 1) == 0);
  uint8_t id = 0;
  uint8_t invalid[20] = {'A', 'C', 2, 41, 0, 1, 'X'};
  assert(catalog.accept(invalid, 7, id, changed) == AnimationCatalog::INVALID);
  invalid[2] = 1;
  invalid[5] = 33;
  assert(catalog.accept(invalid, 7, id, changed) == AnimationCatalog::INVALID);
  puts("Catalog: defaults, fragments, atomic updates, bounds, restore and sparse navigation OK");
}
