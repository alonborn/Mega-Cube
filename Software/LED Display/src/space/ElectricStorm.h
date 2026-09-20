#ifndef ELECTRICSTORM_H
#define ELECTRICSTORM_H

#include "Animation.h"

class ElectricStorm : public Animation {
 private:
  static const uint8_t MAX_SEGMENTS = 120;
  static const uint8_t IMPACT_PARTICLES = 180;
  static constexpr float DURATION = 30.0f;
  static constexpr float STRIKE_DURATION = 0.34f;

  struct Segment {
    Vector3 start;
    Vector3 end;
    float reveal;
    uint8_t branch;
  };

  Segment segments[MAX_SEGMENTS];
  Particle impact_particles[IMPACT_PARTICLES];
  uint8_t segment_count = 0;
  uint8_t strikes_until_impact = 0;
  Vector3 source;
  Vector3 target;
  float age = 0.0f;
  float strike_age = STRIKE_DURATION;
  float next_strike_delay = 0.0f;
  bool strike_active = false;
  bool impact_pending = false;
  bool impact_triggered = false;

  void addSegment(const Vector3 &start, const Vector3 &end, float reveal,
                  uint8_t branch) {
    if (segment_count >= MAX_SEGMENTS) return;
    segments[segment_count++] = {start, end, reveal, branch};
  }

  void addBranch(const Vector3 &origin, const Vector3 &main_direction,
                 float reveal, uint8_t depth) {
    Vector3 point = origin;
    Vector3 direction =
        (main_direction * 0.3f +
         Vector3(noise.nextRandom(-1.0f, 1.0f),
                 noise.nextRandom(-0.25f, 0.65f),
                 noise.nextRandom(-1.0f, 1.0f)))
            .normalize();

    const uint8_t branch_segments = random(2, 5);
    for (uint8_t i = 0; i < branch_segments; ++i) {
      Vector3 next = point + direction * noise.nextRandom(0.9f, 1.8f) +
                     Vector3(noise.nextRandom(-0.4f, 0.4f),
                             noise.nextRandom(-0.25f, 0.25f),
                             noise.nextRandom(-0.4f, 0.4f));
      addSegment(point, next, reveal + i * 0.025f, depth);
      point = next;
    }
  }

  void generateBolt(const Vector3 &bolt_source, const Vector3 &bolt_target,
                    float reveal_offset) {
    const Vector3 main_direction = (bolt_target - bolt_source).normalize();
    Vector3 previous = bolt_source;
    const uint8_t main_segments = 14;

    for (uint8_t i = 1; i <= main_segments; ++i) {
      const float ratio = i / static_cast<float>(main_segments);
      Vector3 next = bolt_source + (bolt_target - bolt_source) * ratio;
      if (i < main_segments) {
        const float jitter = 1.2f * sinf(ratio * PI);
        next.x += noise.nextRandom(-jitter, jitter);
        next.z += noise.nextRandom(-jitter, jitter);
        next.y += noise.nextRandom(-0.25f, 0.25f);
      }
      addSegment(previous, next, ratio + reveal_offset, 0);
      previous = next;

      if (i > 2 && i < main_segments - 1 && random(0, 100) < 45) {
        addBranch(next, main_direction, ratio + reveal_offset, 1);
        if (random(0, 100) < 22) {
          addBranch(next, -main_direction, ratio + reveal_offset, 2);
        }
      }
    }
  }

  void generateStrike() {
    segment_count = 0;
    source = Vector3(noise.nextRandom(-6.5f, 6.5f), 7.0f,
                     noise.nextRandom(-6.5f, 6.5f));
    target = Vector3(noise.nextRandom(-6.5f, 6.5f), -7.1f,
                     noise.nextRandom(-6.5f, 6.5f));
    generateBolt(source, target, 0.0f);

    if (random(0, 100) < 35) {
      const Vector3 second_source(noise.nextRandom(-6.5f, 6.5f), 7.0f,
                                  noise.nextRandom(-6.5f, 6.5f));
      const Vector3 second_target(noise.nextRandom(-6.5f, 6.5f), -7.1f,
                                  noise.nextRandom(-6.5f, 6.5f));
      generateBolt(second_source, second_target,
                   noise.nextRandom(0.06f, 0.17f));
    }

    impact_pending = --strikes_until_impact == 0;
    if (impact_pending) strikes_until_impact = random(3, 7);
    impact_triggered = false;
    strike_active = true;
    strike_age = 0.0f;
    next_strike_delay = noise.nextRandom(0.25f, 1.35f);
  }

  void createImpact() {
    impact_triggered = true;
    for (uint8_t i = 0; i < IMPACT_PARTICLES; ++i) {
      Vector3 velocity(noise.nextRandom(-9.0f, 9.0f),
                       noise.nextRandom(4.0f, 12.0f),
                       noise.nextRandom(-9.0f, 9.0f));
      impact_particles[i] =
          Particle(target, velocity, static_cast<uint8_t>(20 + random(0, 38)),
                   1.0f, noise.nextRandom(0.7f, 1.7f));
    }
  }

  void drawImpact(float dt) {
    const Vector3 gravity(0, -11.0f, 0);
    for (uint8_t i = 0; i < IMPACT_PARTICLES; ++i) {
      Particle &particle = impact_particles[i];
      if (particle.brightness <= 0.0f) continue;
      particle.move(dt, gravity);
      particle.brightness =
          max(0.0f, particle.brightness - dt / particle.seconds);
      Color color(particle.hue, LavaPalette);
      if (random(0, 12) == 0) color = Color::WHITE;
      voxel_add(particle.position,
                color.scale(static_cast<uint8_t>(particle.brightness * 255)));
    }
  }

 public:
  void init() override {
    state = state_t::RUNNING;
    age = 0.0f;
    strike_age = STRIKE_DURATION;
    next_strike_delay = 0.0f;
    strike_active = false;
    strikes_until_impact = random(3, 7);
    for (uint8_t i = 0; i < IMPACT_PARTICLES; ++i) {
      impact_particles[i].brightness = 0.0f;
    }
    setMotionBlur(85);
  }

  void draw(float dt) override {
    age += dt;
    strike_age += dt;
    if (!strike_active && strike_age >= next_strike_delay) generateStrike();

    if (strike_active) {
      const float progress = min(1.0f, strike_age / STRIKE_DURATION);
      const float reveal = min(1.25f, progress * 1.45f);
      const uint8_t brightness = static_cast<uint8_t>(
          255.0f * (progress < 0.78f ? 1.0f : (1.0f - progress) / 0.22f));

      for (uint8_t i = 0; i < segment_count; ++i) {
        const Segment &segment = segments[i];
        if (segment.reveal > reveal) continue;
        Color color = segment.branch == 0 ? Color(190, 230, 255)
                                          : Color(55, 85, 255);
        if (segment.branch == 2) color = Color(155, 45, 255);
        line(segment.start, segment.end, color.scale(brightness));
      }

      radiate4(source, Color::WHITE.scaled(brightness), 1.15f);
      if (reveal >= 0.96f) {
        radiate4(target, Color(80, 165, 255).scaled(brightness), 1.25f);
        if (impact_pending && !impact_triggered) createImpact();
      }

      if (progress >= 1.0f) {
        strike_active = false;
        strike_age = 0.0f;
      }
    }

    drawImpact(dt);
    if (age >= DURATION) state = state_t::INACTIVE;
  }
};

#endif
