#include <Arduino.h>

#include "core/Config.h"
#include "core/ESP8266.h"
#include "space/Animation.h"

Config config;

static Timer print_interval = 2.0f;

static void configureAnimationShow() {
  config.animation.play_one = true;
  config.animation.animation = 16;

  config.animation.accelerometer.runtime = 15.0f;
  config.animation.arrows.runtime = 15.0f;
  config.animation.atoms.runtime = 15.0f;
  config.animation.cube.runtime = 15.0f;
  config.animation.fireworks.runtime = 15.0f;
  config.animation.helix.runtime = 15.0f;
  config.animation.life.runtime = 15.0f;
  config.animation.mario.runtime = 15.0f;
  config.animation.plasma.runtime = 15.0f;
  config.animation.pong.runtime = 15.0f;
  config.animation.arc_scroller.runtime = 15.0f;
  config.animation.box_scroller.runtime = 15.0f;
  config.animation.sinus.runtime = 15.0f;
  config.animation.spectrum.runtime = 15.0f;
  config.animation.starfield.runtime = 15.0f;
  config.animation.twinkels.runtime = 15.0f;
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(115200);
  Serial1.begin(460800);

  DMAMEM static char read_buffer[4096];
  Serial1.addMemoryForRead(read_buffer, sizeof(read_buffer));

  DMAMEM static char write_buffer[1024];
  Serial1.addMemoryForWrite(write_buffer, sizeof(write_buffer));

  ESP8266::request_time();

  configureAnimationShow();

  Animation::begin();
}

void loop() {
  Animation::loop();
  ESP8266::loop();

  if (print_interval.update()) {
    Serial.printf("FPS=%1.2f\n", Animation::fps());
  }
}
