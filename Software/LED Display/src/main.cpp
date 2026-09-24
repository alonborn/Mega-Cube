#include <Arduino.h>

#include "core/Config.h"
#include "power/Timer.h"
#include "space/Animation.h"

Config config;

static Timer print_interval = 2.0f;
static String controller_line;

static void controllerReply(const String& message) {
  Serial1.println(message);
  Serial.println(message);
}

static void handleControllerCommand(String command) {
  command.trim();
  if (command.length() == 0) return;

  if (command.startsWith("ANIMATION ")) {
    const int id = command.substring(10).toInt();
    if (id < 0 || id > 255 || !Animation::get_item(id).object) {
      controllerReply("ERROR INVALID_ANIMATION");
      return;
    }
    config.animation.playlist = false;
    config.animation.play_one = true;
    config.animation.animation = id;
    config.animation.changed = true;
    controllerReply("OK ANIMATION " + String(id));
    return;
  }

  if (command == "PLAYLIST") {
    config.animation.play_one = false;
    config.animation.playlist = true;
    config.animation.changed = true;
    controllerReply("OK PLAYLIST");
    return;
  }

  if (command.startsWith("BRIGHTNESS ")) {
    const int value = constrain(command.substring(11).toInt(), 0, 255);
    config.power.brightness = value / 255.0f;
    Display::setBrightness(value);
    controllerReply("OK BRIGHTNESS " + String(value));
    return;
  }

  if (command == "STATUS") {
    const String mode = config.animation.playlist ? "PLAYLIST" : "ANIMATION";
    controllerReply("STATUS " + mode + " " +
                    String(config.animation.animation) + " " +
                    String(Display::getBrightness()));
    return;
  }

  controllerReply("ERROR UNKNOWN_COMMAND");
}

static void pollController() {
  while (Serial1.available()) {
    const char value = Serial1.read();
    if (value == '\n') {
      handleControllerCommand(controller_line);
      controller_line = "";
    } else if (value != '\r' && controller_line.length() < 180) {
      controller_line += value;
    }
  }
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(115200);
  Serial1.begin(115200);

  config.animation.playlist = true;
  config.animation.play_one = false;
  Animation::begin();
  Serial.println("FlexIO DMA: animation playlist");
}

void loop() {
  pollController();
  Animation::loop();

  if (print_interval.update()) {
    Serial.printf("Playlist FPS=%1.2f DMA_ERR=%lx SHIFTERR=%lx\n",
                  Animation::fps(), (unsigned long)DMA_ERR,
                  (unsigned long)(IMXRT_FLEXIO2_S.SHIFTERR & 0x0F));
  }
}
