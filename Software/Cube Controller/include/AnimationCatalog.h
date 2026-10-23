#pragma once

#include <stdint.h>
#include <stddef.h>
#include <string.h>

class AnimationCatalog {
 public:
  static constexpr size_t MAX_NAME = 32;
  static constexpr uint16_t DEFAULT_COUNT = 40;
  enum Result { INVALID, PARTIAL, COMPLETE };

  const char* name(uint8_t id) const {
    static const char* const defaults[] = {
        "LED TEST", "ACCELEROMETER", "ARROWS", "ATOMS", "CUBE", "FIREWORKS",
        "HELIX 1", "LIFE", "MARIO", "PLASMA", "PONG", "SCROLLER", "SINUS",
        "SPECTRUM", "STARFIELD", "FAIRY LIGHTS", "MULTI LIGHTS", "CHANNEL TEST",
        "CHANNEL COLORS", "THE MATRIX", "SPOTTED SPHERE", "SUPERNOVA", "AURORA",
        "BLACK HOLE", "METABALLS", "ELECTRIC STORM", "DNA TUNNEL", "EYE OF SAURON",
        "WHITE TEST", "RED TEST", "OPENING", "OPENING 2", "BLACKOUT", "HELIX 2",
        "LIGHT CHASE", "DELOREAN FLIGHT", "UNIVERSAL", "YODA", "MUSIC BALLS", "MUSIC EQ"};
    static_assert(sizeof(defaults) / sizeof(defaults[0]) == DEFAULT_COUNT,
                  "Update DEFAULT_COUNT when extending the built-in catalog");
    return id < DEFAULT_COUNT ? defaults[id] : extra[id - DEFAULT_COUNT];
  }

  uint8_t adjacent(uint8_t id, int direction) const {
    for (int step = 1; step <= 256; ++step) {
      const uint8_t candidate = static_cast<uint8_t>(id + direction * step);
      if (name(candidate)[0]) return candidate;
    }
    return id;
  }

  bool restore(uint8_t id, const char* value) {
    if (id < DEFAULT_COUNT || !validName(value)) return false;
    strcpy(extra[id - DEFAULT_COUNT], value);
    return true;
  }

  // AC, version, ID, offset, total length, up to 14 ASCII bytes (default BLE MTU).
  Result accept(const uint8_t* packet, size_t size, uint8_t& id, bool& changed) {
    changed = false;
    if (size < 7 || size > 20 || packet[0] != 'A' || packet[1] != 'C' || packet[2] != 1)
      return INVALID;
    const uint8_t offset = packet[4];
    const uint8_t total = packet[5];
    const size_t count = size - 6;
    if (!total || total > MAX_NAME || offset + count > total) return INVALID;
    for (size_t i = 6; i < size; ++i)
      if (packet[i] < 32 || packet[i] > 126) return INVALID;
    if (offset == 0) {
      stagingId = packet[3];
      stagingLength = total;
      received = 0;
    }
    if (packet[3] != stagingId || total != stagingLength || offset != received) return INVALID;
    memcpy(staging + offset, packet + 6, count);
    received += count;
    if (received != total) return PARTIAL;
    staging[total] = '\0';
    id = stagingId;
    if (!validName(staging)) return INVALID;
    if (id >= DEFAULT_COUNT && strcmp(name(id), staging) != 0) {
      strcpy(extra[id - DEFAULT_COUNT], staging);
      changed = true;
    }
    received = stagingLength = 0;
    return COMPLETE;
  }

 private:
  char extra[256 - DEFAULT_COUNT][MAX_NAME + 1] = {};
  char staging[MAX_NAME + 1] = {};
  uint8_t stagingId = 0;
  uint8_t stagingLength = 0;
  uint8_t received = 0;

  static bool validName(const char* value) {
    const size_t length = strlen(value);
    if (!length || length > MAX_NAME) return false;
    bool visible = false;
    for (size_t i = 0; i < length; ++i) {
      if (value[i] < 32 || value[i] > 126) return false;
      visible |= value[i] != ' ';
    }
    return visible;
  }
};
