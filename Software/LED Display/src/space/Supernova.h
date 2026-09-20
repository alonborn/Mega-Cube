#ifndef SUPERNOVA_H
#define SUPERNOVA_H

#include "Animation.h"

class Supernova : public Animation {
 private:
  static const uint16_t PARTICLES = 320;
  static constexpr float COLLAPSE_END = 4.5f;
  static constexpr float FLASH_END = 5.15f;
  static constexpr float EMISSION_DURATION = 10.0f;
  static constexpr float ANIMATION_END = 16.2f;

  Particle particles[PARTICLES];
  float age = 0.0f;
  uint8_t base_hue = 0;
  bool exploded = false;
  uint16_t next_particle = 0;
  float emission_accumulator = 0.0f;

  void spawnParticle(uint16_t index, float min_speed, float max_speed,
                     float min_lifetime, float max_lifetime) {
    Vector3 direction(noise.nextRandom(-1.0f, 1.0f),
                      noise.nextRandom(-1.0f, 1.0f),
                      noise.nextRandom(-1.0f, 1.0f));
    if (direction.magnitude() < 0.05f) direction = Vector3::X;
    direction.normalize();

    particles[index] = Particle(
        direction * noise.nextRandom(0.0f, 0.45f),
        direction * noise.nextRandom(min_speed, max_speed),
        static_cast<uint8_t>(base_hue + random(0, 72)), 1.0f,
        noise.nextRandom(min_lifetime, max_lifetime));
  }

  void explode() {
    exploded = true;
    for (uint16_t i = 0; i < PARTICLES; ++i) {
      spawnParticle(i, 2.8f, 8.5f, 2.0f, 5.0f);
    }
  }

  void drawShell(float radius, float thickness, const Color &color) {
    for (uint8_t x = 0; x < Display::width; ++x) {
      for (uint8_t y = 0; y < Display::height; ++y) {
        for (uint8_t z = 0; z < Display::depth; ++z) {
          Vector3 point(x - CX, y - CY, z - CZ);
          const float distance = point.magnitude();
          const float shell_distance = fabsf(distance - radius);
          if (shell_distance < thickness) {
            voxel(x, y, z,
                  color.scaled(static_cast<uint8_t>(
                      255.0f * (1.0f - shell_distance / thickness))));
          }
        }
      }
    }
  }

 public:
  void init() override {
    state = state_t::RUNNING;
    age = 0.0f;
    base_hue = random(0, 256);
    exploded = false;
    next_particle = 0;
    emission_accumulator = 0.0f;
    setMotionBlur(150);
  }

  void draw(float dt) override {
    age += dt;

    if (age < COLLAPSE_END) {
      const float progress = age / COLLAPSE_END;
      const float pulse = 0.5f + 0.5f * sinf(age * 18.0f);
      const float radius = 5.5f - 4.2f * progress + pulse * 0.45f;
      const uint8_t heat = static_cast<uint8_t>(80 + 175 * progress);
      radiate4(Vector3(0, 0, 0), Color(heat, 35 + heat / 2, 8), radius);
      drawShell(radius, 0.55f,
                Color(static_cast<uint8_t>(base_hue + age * 25),
                      RainbowGradientPalette));
      return;
    }

    if (!exploded) explode();

    if (age < FLASH_END) {
      const float flash_progress =
          (age - COLLAPSE_END) / (FLASH_END - COLLAPSE_END);
      const float radius = 2.0f + 12.0f * flash_progress;
      const uint8_t flash_brightness = static_cast<uint8_t>(
          255.0f * (1.0f - 0.55f * flash_progress));
      radiate(Vector3(0, 0, 0),
              Color::WHITE.scaled(flash_brightness), radius);
    }

    const float explosion_age = age - COLLAPSE_END;
    const float shell_radius = explosion_age * 5.2f;
    if (shell_radius < 13.5f) {
      const uint8_t shell_brightness = static_cast<uint8_t>(
          255.0f * max(0.0f, 1.0f - shell_radius / 13.5f));
      Color shell_color(static_cast<uint8_t>(base_hue + shell_radius * 8),
                        RainbowGradientPalette);
      drawShell(shell_radius, 0.8f, shell_color.scale(shell_brightness));
    }

    const float emission_age = age - FLASH_END;
    if (emission_age >= 0.0f && emission_age < EMISSION_DURATION) {
      const bool emitting = (static_cast<uint8_t>(emission_age) & 1u) == 0;
      if (emitting) {
        emission_accumulator += dt * 150.0f;
        while (emission_accumulator >= 1.0f) {
          spawnParticle(next_particle, 6.0f, 13.0f, 1.2f, 2.4f);
          next_particle = (next_particle + 1) % PARTICLES;
          emission_accumulator -= 1.0f;
        }
      }

      const uint8_t core_brightness = static_cast<uint8_t>(
          (emitting ? 150.0f : 65.0f) +
          (emitting ? 105.0f : 45.0f) *
              (0.5f + 0.5f * sinf(age * 24.0f)));
      radiate4(Vector3(0, 0, 0), Color::WHITE.scaled(core_brightness), 1.5f);
    }

    for (uint16_t i = 0; i < PARTICLES; ++i) {
      Particle &particle = particles[i];
      particle.move(dt);
      particle.velocity *= 1.0f / (1.0f + dt * 0.55f);
      particle.brightness =
          max(0.0f, particle.brightness - dt / particle.seconds);
      if (particle.brightness <= 0.0f) continue;

      Color color(particle.hue, RainbowGradientPalette);
      if (random(0, 18) == 0) color = Color::WHITE;
      voxel_add(particle.position,
                color.scale(static_cast<uint8_t>(particle.brightness * 255)));
    }

    if (age >= ANIMATION_END) state = state_t::INACTIVE;
  }
};

#endif
