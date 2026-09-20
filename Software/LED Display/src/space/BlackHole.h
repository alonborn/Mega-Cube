#ifndef BLACKHOLE_H
#define BLACKHOLE_H

#include "Animation.h"

class BlackHole : public Animation {
 private:
  static const uint16_t DISK_PARTICLES = 300;
  static const uint8_t JET_PARTICLES = 10;
  static constexpr float DURATION = 30.0f;

  struct DiskParticle {
    float angle;
    float radius;
    float height;
    float speed;
    uint8_t hue;
  };

  struct JetParticle {
    float distance;
    float angle;
    float speed;
    int8_t direction;
  };

  DiskParticle disk[DISK_PARTICLES];
  JetParticle jets[JET_PARTICLES];
  float age = 0.0f;

  void resetDiskParticle(uint16_t i, bool fill_disk) {
    disk[i].angle = noise.nextRandom(0.0f, TWO_PI);
    disk[i].radius = fill_disk ? noise.nextRandom(1.7f, 9.5f)
                               : noise.nextRandom(8.0f, 10.0f);
    disk[i].height = noise.nextGaussian(0.0f, 0.38f) *
                     (0.35f + disk[i].radius / 9.5f);
    disk[i].speed = noise.nextRandom(0.8f, 1.35f);
    disk[i].hue = random(0, 48);
  }

  void resetJetParticle(uint8_t i, bool fill_jet) {
    jets[i].distance = fill_jet ? noise.nextRandom(1.5f, 10.5f) : 1.5f;
    jets[i].angle = noise.nextRandom(0.0f, TWO_PI);
    jets[i].speed = noise.nextRandom(4.5f, 8.5f);
    jets[i].direction = (i & 1) ? 1 : -1;
  }

 public:
  void init() override {
    state = state_t::RUNNING;
    age = 0.0f;
    setMotionBlur(145);
    for (uint16_t i = 0; i < DISK_PARTICLES; ++i) {
      resetDiskParticle(i, true);
    }
    for (uint8_t i = 0; i < JET_PARTICLES; ++i) resetJetParticle(i, true);
  }

  void draw(float dt) override {
    age += dt;
    const float cycle = age * TWO_PI / DURATION;
    const Quaternion orientation =
        Quaternion(age * 360.0f / DURATION, Vector3::Y) *
        Quaternion(28.0f + 14.0f * sinf(cycle), Vector3::X) *
        Quaternion(-18.0f + 10.0f * cosf(cycle), Vector3::Z);

    for (uint16_t i = 0; i < DISK_PARTICLES; ++i) {
      DiskParticle &particle = disk[i];
      const float orbit_speed = particle.speed * (1.15f + 7.0f / particle.radius);
      particle.angle += dt * orbit_speed;
      particle.radius -= dt * (0.22f + 0.95f / particle.radius);
      particle.height *= 1.0f / (1.0f + dt * 0.7f);

      if (particle.radius < 1.45f) {
        resetDiskParticle(i, false);
        continue;
      }

      Vector3 position(cosf(particle.angle) * particle.radius,
                       particle.height,
                       sinf(particle.angle) * particle.radius);
      position = orientation.rotate(position);

      Color color;
      if (particle.radius < 3.0f) {
        color = Color(255, 235, 170);
      } else if (particle.radius < 5.5f) {
        color = Color(255, 75 + particle.hue * 2, 10);
      } else {
        color = Color(150 + particle.hue * 2, 15, 210);
      }

      const uint8_t brightness = static_cast<uint8_t>(
          min(255.0f, 105.0f + 240.0f / particle.radius));
      voxel_add(position, color.scale(brightness));
    }

    // A bright photon ring outlines the otherwise dark event horizon.
    for (uint8_t i = 0; i < 72; ++i) {
      const float angle = i * TWO_PI / 72.0f + age * 1.8f;
      const float wobble = 0.16f * sinf(angle * 3.0f + age * 2.2f);
      Vector3 ring(cosf(angle) * (1.75f + wobble), 0,
                   sinf(angle) * (1.75f + wobble));
      Color color = (i % 7 == 0) ? Color::WHITE : Color(255, 165, 35);
      voxel_add(orientation.rotate(ring), color);
    }

    // Bipolar jets leave the center along the tilted rotation axis.
    for (uint8_t i = 0; i < JET_PARTICLES; ++i) {
      JetParticle &jet = jets[i];
      jet.distance += dt * jet.speed;
      jet.angle += dt * 4.0f;
      if (jet.distance > 11.5f) resetJetParticle(i, false);

      const float spread = 0.08f * jet.distance;
      Vector3 position(cosf(jet.angle) * spread,
                       jet.direction * jet.distance,
                       sinf(jet.angle) * spread);
      const uint8_t brightness = static_cast<uint8_t>(
          max(45.0f, 255.0f - jet.distance * 17.0f));
      voxel_add(orientation.rotate(position),
                Color(45, 150, 255).scale(brightness));
    }

    if (age >= DURATION) state = state_t::INACTIVE;
  }
};

#endif
