#include <Arduino.h>

#include "core/Config.h"
#include "power/Timer.h"
#include "space/Animation.h"

Config config;

static Timer print_interval = 2.0f;

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(115200);

  config.animation.play_one = true;
  config.animation.animation = 12;  // Sinus
  Animation::begin();
  Serial.println("FlexIO DMA: Sinus animation");
}

void loop() {
  Animation::loop();

  if (print_interval.update()) {
    Serial.printf("Sinus FPS=%1.2f DMA_ERR=%lx SHIFTERR=%lx\n",
                  Animation::fps(), (unsigned long)DMA_ERR,
                  (unsigned long)(IMXRT_FLEXIO2_S.SHIFTERR & 0x0F));
  }
}
