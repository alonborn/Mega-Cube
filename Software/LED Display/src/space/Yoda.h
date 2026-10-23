#ifndef YODA_H
#define YODA_H

#include "Animation.h"

class Yoda : public Animation {
 private:
  float age = 0.0f;

  static float smooth(float value) {
    value = constrain(value, 0.0f, 1.0f);
    return value * value * (3.0f - 2.0f * value);
  }

  static bool insideHead(const Vector3 &point) {
    if (point.y < -5.35f || point.y > 5.35f) return false;
    const float profile = sqrtf(max(0.0f, 1.0f -
                                           (point.y * point.y) / (5.35f * 5.35f)));
    float halfWidth = 4.9f * profile + 0.35f;
    float halfDepth = 4.35f * profile + 0.35f;
    if (point.y < -0.5f) halfWidth *= 1.0f - 0.14f * (-point.y / 5.35f);
    return (point.x * point.x) / (halfWidth * halfWidth) +
               (point.z * point.z) / (halfDepth * halfDepth) <=
           1.0f;
  }

  static bool insideEar(const Vector3 &point, float &progress) {
    const float distance = fabsf(point.x) - 3.45f;
    if (distance < 0.0f || distance > 4.05f) return false;
    progress = distance / 4.05f;
    const float centerY = 0.45f + progress * 0.85f;
    const float halfHeight = 0.90f * (1.0f - progress) + 0.13f;
    const float halfDepth = 1.30f * (1.0f - progress) + 0.18f;
    const float dy = (point.y - centerY) / halfHeight;
    const float dz = point.z / halfDepth;
    return dy * dy + dz * dz <= 1.0f;
  }

  Color faceColor(const Vector3 &point, bool ear, float earProgress) const {
    if (ear && earProgress > 0.12f && earProgress < 0.82f &&
        point.z > 0.0f && fabsf(point.y - (0.45f + earProgress * 0.85f)) <
                                  0.48f * (1.0f - earProgress) + 0.08f) {
      return Color(155, 102, 72);
    }

    const float shading = constrain(0.72f + point.z * 0.035f + point.y * 0.018f,
                                    0.52f, 1.0f);
    Color color(24, 150, 36);

    const float leftEye = (point.x + 1.45f) * (point.x + 1.45f) +
                          (point.y - 0.70f) * (point.y - 0.70f) +
                          (point.z - 3.95f) * (point.z - 3.95f);
    const float rightEye = (point.x - 1.45f) * (point.x - 1.45f) +
                           (point.y - 0.70f) * (point.y - 0.70f) +
                           (point.z - 3.95f) * (point.z - 3.95f);
    const float eyeDistance = min(leftEye, rightEye);
    if (eyeDistance < 1.0f) {
      color = Color(210, 155, 35);
      const float pupilX = point.x < 0.0f ? -1.35f : 1.35f;
      const float pupilDistance = (point.x - pupilX) * (point.x - pupilX) +
                                  (point.y - 0.68f) * (point.y - 0.68f) +
                                  (point.z - 4.78f) * (point.z - 4.78f);
      if (pupilDistance < 0.30f) color = Color(18, 12, 5);
      else if (pupilDistance < 0.52f) color = Color(145, 72, 12);
      else if (point.z > 4.45f && point.y > 0.9f && eyeDistance < 0.72f)
        color = Color(235, 210, 135);
    }

    const float nose = point.x * point.x / 0.30f +
                       (point.y - 0.12f) * (point.y - 0.12f) / 0.52f +
                       (point.z - 4.05f) * (point.z - 4.05f) / 0.82f;
    if (nose < 1.0f) color = Color(42, 155, 48);

    if (point.z > 3.35f && point.y < -1.85f && point.y > -2.65f &&
        fabsf(point.x) < 1.45f) {
      color = Color(18, 75, 25);
    }

    if (point.z > 3.0f && point.y > 2.35f && point.y < 3.55f &&
        fabsf(point.x) < 2.4f &&
        (static_cast<int>(point.x * 2.0f) + static_cast<int>(point.y * 2.0f)) % 3 == 0) {
      color = Color(20, 105, 30);
    }

    return color.scaled(static_cast<uint8_t>(255.0f * shading));
  }

 public:
  void init() override {
    state = state_t::RUNNING;
    age = 0.0f;
    setMotionBlur(0);
  }

  void draw(float dt) override {
    age += dt;
    const float rise = smooth(age / 5.0f);
    const float headCenterY = -13.0f + 13.0f * rise;
    const Quaternion rotation(age * 45.0f, Vector3::Y);
    const Quaternion inverseRotation = rotation.inversed();

    for (uint8_t x = 0; x < Display::width; ++x) {
      for (uint8_t y = 0; y < Display::height; ++y) {
        for (uint8_t z = 0; z < Display::depth; ++z) {
          const Vector3 world(x - CX, y - CY - headCenterY, z - CZ);
          const Vector3 local = inverseRotation.rotate(world);
          float earProgress = 0.0f;
          const bool ear = insideEar(local, earProgress);
          const float leftEye = (local.x + 1.45f) * (local.x + 1.45f) +
                                (local.y - 0.70f) * (local.y - 0.70f) +
                                (local.z - 3.95f) * (local.z - 3.95f);
          const float rightEye = (local.x - 1.45f) * (local.x - 1.45f) +
                                 (local.y - 0.70f) * (local.y - 0.70f) +
                                 (local.z - 3.95f) * (local.z - 3.95f);
          const float nose = local.x * local.x / 0.30f +
                             (local.y - 0.12f) * (local.y - 0.12f) / 0.52f +
                             (local.z - 4.05f) * (local.z - 4.05f) / 0.82f;
          if (!ear && !insideHead(local) && min(leftEye, rightEye) >= 1.0f &&
              nose >= 1.0f) continue;

          Color color = faceColor(local, ear, earProgress);
          voxel(x, y, z, color);
        }
      }
    }
  }
};

#endif
