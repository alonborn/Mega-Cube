#include <Arduino.h>

#include "core/Config.h"
#include "core/LiveMusic.h"
#include "power/Timer.h"
#include "space/Animation.h"

Config config;

static Timer print_interval = 2.0f;
static String controller_line;
static int16_t pending_animation_reply = -1;
static bool pending_playlist_reply = false;
static uint16_t audio_latency_ms = 0;

static void controllerReply(const String& message) {
  Serial1.println(message);
  Serial.println(message);
}

void sendAnimationEvent(const char* event) {
  controllerReply("EVENT " + String(event));
}

float getAudioLeadSeconds() {
  return audio_latency_ms / 1000.0f;
}

static void handleControllerCommand(String command) {
  command.trim();
  if (command.length() == 0) return;

  // Replies received on the command input must not create an error echo loop.
  if (command.startsWith("ERROR ") || command.startsWith("OK ") ||
      command.startsWith("EVENT ") || command.startsWith("STATUS ")) {
    static uint32_t last_warning = 0;
    if (millis() - last_warning >= 1000) {
      last_warning = millis();
      Serial.println("UART RX received a reply; check RX/TX wiring");
    }
    return;
  }

  if (command.startsWith("MUSIC ")) {
    if (!config.animation.playlist &&
        (config.animation.animation == ANIMATION_MUSIC_BALLS ||
         config.animation.animation == ANIMATION_MUSIC_EQUALIZER))
      liveMusic().accept(command.c_str() + 6, millis());
    return;
  }

  if (command.startsWith("ANIMATION ")) {
    const int id = command.substring(10).toInt();
    if (id < 0 || id > 255 || !Animation::get_item(id).object) {
      controllerReply("ERROR INVALID_ANIMATION");
      return;
    }
    sendAnimationEvent("STORM_STOP");
    config.animation.playlist = false;
    config.animation.play_one = true;
    config.animation.animation = id;
    config.animation.changed = true;
    pending_animation_reply = id;
    pending_playlist_reply = false;
    return;
  }

  if (command == "PLAYLIST") {
    sendAnimationEvent("STORM_STOP");
    config.animation.play_one = false;
    config.animation.playlist = true;
    config.animation.changed = true;
    pending_animation_reply = -1;
    pending_playlist_reply = true;
    return;
  }

  if (command.startsWith("AUDIO_LATENCY ")) {
    audio_latency_ms = constrain(command.substring(14).toInt(), 0, 1000);
    controllerReply("OK AUDIO_LATENCY " + String(audio_latency_ms));
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

  static uint32_t last_error = 0;
  if (millis() - last_error >= 1000) {
    last_error = millis();
    Serial.printf("UART invalid command: %.40s\n", command.c_str());
    controllerReply("ERROR UNKNOWN_COMMAND");
  }
}

static void pollController() {
  // Bound command processing so a noisy UART can never starve animation frames.
  uint8_t byte_budget = 64;
  static bool discard_line = false;
  while (byte_budget-- > 0 && Serial1.available()) {
    const char value = Serial1.read();
    if (value == '\n') {
      if (!discard_line) handleControllerCommand(controller_line);
      controller_line = "";
      discard_line = false;
    } else if (value != '\r' && !discard_line) {
      if (value < 32 || value > 126 || controller_line.length() >= 180)
        discard_line = true;
      else controller_line += value;
    }
  }
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(115200);
  Serial1.begin(115200);

  config.animation.playlist = false;
  config.animation.play_one = true;
  config.animation.animation = ANIMATION_SINUS;
  config.animation.changed = true;
  Animation::begin();
  Serial.println("FlexIO DMA: controller command mode");
}

void loop() {
  pollController();
  Animation::loop();

  if (!config.animation.changed) {
    if (pending_animation_reply >= 0) {
      controllerReply("OK ANIMATION " + String(pending_animation_reply));
      pending_animation_reply = -1;
    } else if (pending_playlist_reply) {
      controllerReply("OK PLAYLIST");
      pending_playlist_reply = false;
    }
  }

  if (print_interval.update()) {
    Serial.printf("Playlist FPS=%1.2f DMA_ERR=%lx SHIFTERR=%lx\n",
                  Animation::fps(), (unsigned long)DMA_ERR,
                  (unsigned long)(IMXRT_FLEXIO2_S.SHIFTERR & 0x0F));
    if (!config.animation.playlist &&
        (config.animation.animation == ANIMATION_MUSIC_BALLS ||
         config.animation.animation == ANIMATION_MUSIC_EQUALIZER))
      Serial.printf("Music frames=%lu onsets=%lu locked=%u BPM=%.1f\n",
                    static_cast<unsigned long>(liveMusic().receivedFrames),
                    static_cast<unsigned long>(liveMusic().receivedOnsets),
                    liveMusic().rhythmStable ? 1 : 0, liveMusic().tempoBpm());
  }
}
