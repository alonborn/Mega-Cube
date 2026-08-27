#include <Arduino.h>

#include "core/Config.h"
#include "core/ESP8266.h"
#include "space/Animation.h"

Config config;

static Timer print_interval = 2.0f;

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(115200);
  Serial1.begin(460800);

  DMAMEM static char read_buffer[4096];
  Serial1.addMemoryForRead(read_buffer, sizeof(read_buffer));

  DMAMEM static char write_buffer[1024];
  Serial1.addMemoryForWrite(write_buffer, sizeof(write_buffer));

  ESP8266::request_time();

  Animation::begin();
}

void loop() {
  Animation::loop();
  ESP8266::loop();

  if (print_interval.update()) {
    Serial.printf("FPS=%1.2f\n", Animation::fps());
  }
}
