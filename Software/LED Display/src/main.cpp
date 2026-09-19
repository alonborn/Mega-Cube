#include <Arduino.h>
#include <math.h>

#include "core/Display.h"
#include "core/Graphics.h"
#include "power/Math8.h"

static constexpr float RESOLUTION = 30.0f;
static constexpr float RADIUS = 7.5f;
static constexpr float PHASE_SPEED = PI;
static constexpr int16_t HUE_SPEED = -50 * 255;
static constexpr uint8_t BRIGHTNESS = 200;
static constexpr bool RUN_CHANNEL_TEST = false;
static constexpr bool RUN_DEPTH_TEST = true;
static constexpr uint32_t CHANNEL_INTERVAL_MS = 2000;
static constexpr uint32_t PLANE_INTERVAL_MS = 3000;

static float phase = 0.0f;
static int16_t hue16 = 0;
static uint32_t previous_frame_us = 0;
static uint32_t fps_started_ms = 0;
static uint32_t frame_count = 0;
static uint32_t channel_started_ms = 0;
static uint8_t current_channel = 30;
static bool channel_frame_pending = true;
static uint32_t plane_started_ms = 0;
static uint8_t current_plane = 15;
static bool plane_frame_pending = true;

static void drawWave(float dt) {
  phase += dt * PHASE_SPEED;
  hue16 += static_cast<int16_t>(dt * HUE_SPEED);

  const Quaternion rotation(phase * 10.0f, Vector3(1, 1, 1));
  for (uint16_t x = 0; x <= RESOLUTION; ++x) {
    const float xprime = mapf(x, 0, RESOLUTION, -2, 2);
    for (uint16_t z = 0; z <= RESOLUTION; ++z) {
      const float zprime = mapf(z, 0, RESOLUTION, -2, 2);
      const float y = sinf(phase + sqrtf(xprime * xprime + zprime * zprime));
      Vector3 point(2 * (x / RESOLUTION) - 1,
                    2 * (z / RESOLUTION) - 1,
                    y);
      point = rotation.rotate(point) * RADIUS;
      Color color((hue16 >> 8) + static_cast<int8_t>(y * 64),
                  RainbowGradientPalette);
      radiate(point, color.scale(BRIGHTNESS), 1.0f);
    }
  }
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(115200);

  Display::begin();
  Display::setMotionBlur(0);
  Display::setBrightness(255);
  previous_frame_us = micros();
  fps_started_ms = millis();
  channel_started_ms = millis();
  plane_started_ms = millis();
  if (RUN_CHANNEL_TEST) {
    Serial.println("FlexIO DMA: physical channel test 30/31");
  } else if (RUN_DEPTH_TEST) {
    Serial.println("FlexIO DMA: rear-to-front Z plane test");
  } else {
    Serial.println("FlexIO DMA: 3D sine wave");
  }
}

void loop() {
  if (RUN_CHANNEL_TEST) {
    const uint32_t now_ms = millis();
    if (now_ms - channel_started_ms >= CHANNEL_INTERVAL_MS) {
      channel_started_ms += CHANNEL_INTERVAL_MS;
      current_channel = current_channel == 30 ? 31 : 30;
      channel_frame_pending = true;
    }
    if (channel_frame_pending && Display::available()) {
      Display::testChannel(current_channel, 0xFFFFFF00u);
      Serial.printf("CHANNEL %u\n", current_channel);
      channel_frame_pending = false;
    }
    return;
  }

  if (RUN_DEPTH_TEST) {
    const uint32_t now_ms = millis();
    if (now_ms - plane_started_ms >= PLANE_INTERVAL_MS) {
      plane_started_ms += PLANE_INTERVAL_MS;
      current_plane = current_plane == 0 ? 15 : current_plane - 1;
      plane_frame_pending = true;
    }
    if (plane_frame_pending && Display::available()) {
      Display::clear();
      const Color plane_color =
          current_plane == 15 ? Color::RED : Color::WHITE;
      for (uint8_t x = 0; x < Display::width; ++x) {
        for (uint8_t y = 0; y < Display::height; ++y) {
          voxel(x, y, current_plane, plane_color);
        }
      }
      Display::update();
      Serial.printf("Z PLANE %u\n", current_plane);
      plane_frame_pending = false;
    }
    return;
  }

  if (!Display::available()) return;

  const uint32_t now_us = micros();
  const float dt = (now_us - previous_frame_us) * 0.000001f;
  previous_frame_us = now_us;

  Display::clear();
  drawWave(dt);
  Display::update();
  ++frame_count;

  const uint32_t now_ms = millis();
  if (now_ms - fps_started_ms >= 2000) {
    const float fps = frame_count * 1000.0f / (now_ms - fps_started_ms);
    Serial.printf("3D sine wave FPS=%1.2f DMA_ERR=%lx SHIFTERR=%lx\n",
                  fps, (unsigned long)DMA_ERR,
                  (unsigned long)(IMXRT_FLEXIO2_S.SHIFTERR & 0x0F));
    frame_count = 0;
    fps_started_ms = now_ms;
  }
}
