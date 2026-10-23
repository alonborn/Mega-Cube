#ifndef LIVE_MUSIC_H
#define LIVE_MUSIC_H

#include <math.h>
#include <stdint.h>
#include <string.h>

class LiveMusicInput {
 private:
  float targetLevel = 0.0f;
  float targets[8] = {};
  uint32_t updatedAt = 0;
  uint8_t lastSequence = 0;
  uint8_t lastBeat = 0;
  bool valid = false;
  uint32_t onsetTimes[16] = {};
  uint8_t onsetCount = 0;
  uint32_t beatAt = 0;
  uint32_t pulseAt = 0;
  float periodMs = 0.0f;
  bool beatSeen = false;
  float pendingPeriod = 0.0f;
  uint32_t pendingAnchor = 0;
  uint8_t pendingFits = 0;

  void clearRhythm() {
    onsetCount = 0;
    beatSeen = false;
    rhythmStable = false;
    rhythmPulse = 0.0f;
    periodMs = 0.0f;
    pendingPeriod = 0.0f;
    pendingFits = 0;
  }

  void trackRhythm(uint32_t now) {
    if (rhythmStable && now - beatAt > periodMs * 8.0f) clearRhythm();
    for (int i = 15; i > 0; --i) onsetTimes[i] = onsetTimes[i - 1];
    onsetTimes[0] = now;
    if (onsetCount < 16) ++onsetCount;

    if (rhythmStable) {
      const float interval = now - beatAt;
      const int steps = static_cast<int>(interval / periodMs + 0.5f);
      if (steps >= 1 && steps <= 8 &&
          fabsf(interval - steps * periodMs) <= fmaxf(40.0f, periodMs * 0.18f)) {
        periodMs += (interval / steps - periodMs) * 0.06f;
        beatAt = now;
        // A late onset corrects the grid without flashing twice for one beat.
        if (now - pulseAt > periodMs * 0.5f) rhythmPulse = 1.0f;
        pulseAt = now;
      }
      return;
    }
    if (onsetCount < 7) return;

    // Fit a beat grid to recent onsets; off-beat notes need not reset the clock.
    float ages[16];
    uint8_t count = 0;
    for (uint8_t i = 0; i < onsetCount; ++i) {
      const uint32_t age = now - onsetTimes[i];
      if (age <= 10000) ages[count++] = age;
    }
    if (count < 7) return;
    float bestScore = 0.0f;
    float bestPeriod = 0.0f;
    uint8_t bestAnchor = 0;
    for (uint8_t anchor = 0; anchor < count && anchor < 4; ++anchor) {
      for (uint8_t other = 0; other < count; ++other) {
        const float span = fabsf(ages[other] - ages[anchor]);
        for (uint8_t divisor = 1; divisor <= 8; ++divisor) {
          const float candidate = span / divisor;
          if (candidate < 250.0f || candidate > 1600.0f) continue;
          const float tolerance = fmaxf(35.0f, candidate * 0.15f);
          bool slots[48] = {};
          int first = 48;
          int last = 0;
          uint8_t matches = 0;
          float quality = 0.0f;
          for (uint8_t i = 0; i < count; ++i) {
            const float relative = ages[i] - ages[anchor];
            const float position = relative / candidate;
            const int step = static_cast<int>(position + (position < 0 ? -0.5f : 0.5f));
            const int slot = step + 8;
            const float error = fabsf(relative - step * candidate);
            if (slot < 0 || slot >= 48 || error > tolerance || slots[slot]) continue;
            slots[slot] = true;
            if (slot < first) first = slot;
            if (slot > last) last = slot;
            ++matches;
            quality += 1.0f - error / tolerance;
          }
          if (matches < 6 || matches < count * 0.6f || quality < matches * 0.7f) continue;
          const float coverage = matches / static_cast<float>(last - first + 1);
          if (coverage < 0.7f) continue;
          const float score = matches * 10.0f + quality * 2.0f + coverage;
          if (score > bestScore) {
            bestScore = score;
            bestPeriod = candidate;
            bestAnchor = anchor;
          }
        }
      }
    }
    if (bestPeriod == 0.0f) {
      pendingFits = 0;
      return;
    }
    bool consistent = false;
    if (pendingFits > 0 && fabsf(bestPeriod - pendingPeriod) <= pendingPeriod * 0.12f) {
      const float offset = static_cast<float>(now - pendingAnchor) - ages[bestAnchor];
      const float position = offset / pendingPeriod;
      const int step = static_cast<int>(position + (position < 0 ? -0.5f : 0.5f));
      consistent = fabsf(offset - step * pendingPeriod) <= fmaxf(40.0f, pendingPeriod * 0.18f);
    }
    pendingFits = consistent ? pendingFits + 1 : 1;
    pendingPeriod = bestPeriod;
    pendingAnchor = now - static_cast<uint32_t>(ages[bestAnchor]);
    if (pendingFits < 3) return;
    periodMs = bestPeriod;
    beatAt = now - static_cast<uint32_t>(ages[bestAnchor]);
    pulseAt = beatAt + static_cast<uint32_t>(floorf((now - beatAt) / periodMs) * periodMs);
    beatSeen = rhythmStable = true;
    const float phase = (now - beatAt) / periodMs;
    const int step = static_cast<int>(phase + 0.5f);
    if (fabsf(phase - step) <= 0.15f) {
      beatAt = now;
      pulseAt = now;
      rhythmPulse = 1.0f;
    }
  }

  static int hexDigit(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return -1;
  }

  static float follow(float value, float target, float dt) {
    const float time = target > value ? 0.008f : 0.12f;
    return value + (target - value) * (1.0f - expf(-dt / time));
  }

  static float response(uint8_t value) {
    return powf(value / 255.0f, RESPONSE_EXPONENT);
  }

 public:
  static constexpr float RESPONSE_EXPONENT = 1.8f;
  float level = 0.0f;
  float bands[8] = {};
  float beat = 0.0f;
  bool rhythmStable = false;
  float rhythmPulse = 0.0f;
  uint32_t receivedFrames = 0;
  uint32_t receivedOnsets = 0;

  float tempoBpm() const { return rhythmStable ? 60000.0f / periodMs : 0.0f; }

  void reset() { *this = LiveMusicInput(); }

  // Twelve bytes: frame sequence, level, onset, beat counter, eight bands.
  bool accept(const char* hex, uint32_t now) {
    if (strlen(hex) != 24) return false;
    uint8_t data[12];
    for (uint8_t i = 0; i < 12; ++i) {
      const int high = hexDigit(hex[i * 2]);
      const int low = hexDigit(hex[i * 2 + 1]);
      if (high < 0 || low < 0) return false;
      data[i] = (high << 4) | low;
    }
    if (valid && now - updatedAt < 500 && data[0] == lastSequence) return false;
    ++receivedFrames;
    if (valid && data[3] != lastBeat) {
      beat = fmaxf(beat, data[2] / 255.0f);
      receivedOnsets += static_cast<uint8_t>(data[3] - lastBeat);
      if (data[2] > 0) trackRhythm(now);
    }
    lastSequence = data[0];
    lastBeat = data[3];
    targetLevel = response(data[1]);
    for (uint8_t i = 0; i < 8; ++i) targets[i] = response(data[4 + i]);
    updatedAt = now;
    valid = true;
    return true;
  }

  void advance(float dt, uint32_t now) {
    dt = fmaxf(0.0f, fminf(dt, 0.1f));
    const bool fresh = valid && now - updatedAt < 500;
    if (!fresh || (beatSeen && now - beatAt > periodMs * 8.0f))
      clearRhythm();
    level = follow(level, fresh ? targetLevel : 0.0f, dt);
    for (uint8_t i = 0; i < 8; ++i)
      bands[i] = follow(bands[i], fresh ? targets[i] : 0.0f, dt);
    beat *= expf(-dt / 0.16f);
    rhythmPulse *= expf(-dt / 0.045f);
    if (rhythmStable && targetLevel > 0.02f && now - pulseAt >= periodMs) {
      const uint32_t steps = static_cast<uint32_t>((now - pulseAt) / periodMs);
      pulseAt += static_cast<uint32_t>(steps * periodMs);
      // Preserve the grid phase even if a display frame arrives late.
      rhythmPulse = expf(-static_cast<float>(now - pulseAt) / 45.0f);
    }
  }
};

inline LiveMusicInput& liveMusic() {
  static LiveMusicInput input;
  return input;
}

#endif
