#ifndef FIREWORKS_H
#define FIREWORKS_H

#include "Animation.h"

void sendAnimationEvent(const char* event);
float getAudioLeadSeconds();

class Fireworks : public Animation {
 private:
  static constexpr uint16_t MAX_DEBRIS = 320;
  float radius;
  uint16_t numDebris;
  Vector3 source;
  Vector3 target;
  Vector3 velocity;
  Vector3 gravity;
  Particle missile;
  Particle debris[MAX_DEBRIS];
  boolean exploded;
  boolean waiting;
  boolean pendingExplosion;
  uint8_t pendingExplosionCount;
  Vector3 pendingExplosionPosition;
  Timer launchDelay;
  Timer explosionDelay;

  static constexpr auto &settings = config.animation.fireworks;

  void scheduleNext() {
    waiting = true;
    launchDelay = noise.nextRandom(0.1f, 1.0f);
  }

  void createExplosion(const Vector3 &position, uint8_t explosionCount) {
    pendingExplosion = false;
    exploded = true;
    const uint16_t debrisPerExplosion = random(70, 101);
    numDebris = min<uint16_t>(debrisPerExplosion * explosionCount, MAX_DEBRIS);
    const float pwr = noise.nextRandom(0.50f, 1.00f);
    const uint8_t baseHue = random(0, 256);
    Vector3 centers[4];
    for (uint8_t cluster = 0; cluster < explosionCount; ++cluster) {
      centers[cluster] =
          position + Vector3(noise.nextRandom(-0.45f, 0.45f),
                             noise.nextRandom(-0.20f, 0.30f),
                             noise.nextRandom(-0.45f, 0.45f));
    }
    for (uint16_t i = 0; i < numDebris; ++i) {
      const uint8_t cluster = i % explosionCount;
      Vector3 speed(noise.nextRandom(-pwr, pwr),
                    noise.nextRandom(-pwr, pwr),
                    noise.nextRandom(-pwr, pwr));
      const uint8_t hue = baseHue + cluster * (256 / explosionCount) +
                          random(0, 36);
      debris[i] = {centers[cluster], speed, hue, 1.0f,
                   noise.nextRandom(1.0f, 2.0f)};
    }
  }

  void beginExplosion(const Vector3 &position) {
    pendingExplosionCount = random(0, 8) == 0 ? random(3, 5) : 1;
    if (pendingExplosionCount == 1)
      sendAnimationEvent("EXPLOSION");
    else if (pendingExplosionCount == 3)
      sendAnimationEvent("EXPLOSION_3");
    else
      sendAnimationEvent("EXPLOSION_4");

    const float lead = getAudioLeadSeconds();
    if (lead > 0.001f) {
      pendingExplosion = true;
      pendingExplosionPosition = position;
      explosionDelay = lead;
    } else {
      createExplosion(position, pendingExplosionCount);
    }
  }

 public:
  void init() {
    state = state_t::RUNNING;
    timer_running = settings.runtime;
    radius = settings.radius;
    exploded = false;
    pendingExplosion = false;
    scheduleNext();
  }

  void fireArrow() {
    waiting = false;
    pendingExplosion = false;
    if (random(0, 2) == 0) sendAnimationEvent("LAUNCH");
    source = Vector3(noise.nextGaussian(0.0f, 0.25f), -1.0f,
                     noise.nextGaussian(0.0f, 0.25f));
    target = Vector3(noise.nextGaussian(0.0f, 0.25f),
                     noise.nextGaussian(0.8f, 0.10f),
                     noise.nextGaussian(0.0f, 0.25f));
    const float t = noise.nextGaussian(0.60f, 0.20f);
    velocity = (target - source) / t;
    missile.position = source;
    missile.velocity = velocity;
    gravity = Vector3(0, -1.0f, 0);
    exploded = false;
  }

  void draw(float dt) {
    radius = settings.radius;
    setMotionBlur(settings.motionBlur);
    const uint8_t brightness = settings.brightness * getBrightness();

    if (waiting) {
      if (launchDelay.update()) fireArrow();
      return;
    }

    if (pendingExplosion) {
      if (explosionDelay.update())
        createExplosion(pendingExplosionPosition, pendingExplosionCount);
      return;
    }

    if (!exploded) {
      Vector3 previous = missile.position;
      missile.move(dt, gravity);
      if ((previous.y > missile.position.y) | (missile.position.y > target.y))
        beginExplosion(previous);
      else
        voxel(missile.position * radius, Color::WHITE);
    }

    if (exploded) {
      uint16_t visible = 0;
      for (uint16_t i = 0; i < numDebris; ++i) {
        if (debris[i].position.y > -1.0f)
          debris[i].move(dt, gravity);
        else
          debris[i].position.y = -1.0f;
        if (debris[i].brightness > 0) {
          visible++;
          debris[i].brightness -= dt * (1 / debris[i].seconds);
        } else {
          debris[i].brightness = 0;
        }
        Color color = Color(debris[i].hue, RainbowGradientPalette);
        if (random(0, 20) == 0) color = Color::WHITE;
        color.scale(debris[i].brightness * brightness);
        voxel_add(debris[i].position * radius, color);
      }
      if (timer_running.update()) state = state_t::ENDING;
      if (visible == 0) {
        if (state == state_t::ENDING)
          state = state_t::INACTIVE;
        else
          scheduleNext();
      }
    }
  }
};
#endif
