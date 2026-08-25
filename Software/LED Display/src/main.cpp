#include <Arduino.h>

static const uint8_t DIN = 8;
static const uint8_t WCK = 9;
static const uint8_t BCK = 10;
static const uint16_t LEDS_PER_CHANNEL = 128;
static const uint8_t BITS_PER_LED = 24;
static uint8_t frame = 0;

static inline void latchAllChannels(bool high) {
  for (uint8_t i = 0; i < 32; i++) {
    digitalWriteFast(DIN, high ? HIGH : LOW);
    digitalWriteFast(BCK, HIGH);
    digitalWriteFast(BCK, LOW);
  }

  digitalWriteFast(WCK, HIGH);
  digitalWriteFast(WCK, LOW);
}

static void sendPulse(bool value) {
  // PL9823 waveform:
  // 0-bit = high, low,  low,  low
  // 1-bit = high, data, data, low
  latchAllChannels(true);
  latchAllChannels(value);
  latchAllChannels(value);
  latchAllChannels(false);
}

static void sendPl9823Frame() {
  noInterrupts();
  for (uint16_t led = 0; led < LEDS_PER_CHANNEL; led++) {
    const bool active_led = ((led + frame) & 0x0F) < 8;
    const uint8_t red = active_led ? 24 : 0;
    const uint8_t green = active_led ? 0 : 24;
    const uint8_t blue = ((led >> 4) & 1) ? 24 : 0;
    const uint32_t color = ((uint32_t)red << 16) |
                           ((uint32_t)green << 8) |
                           blue;

    for (uint8_t bit = 0; bit < BITS_PER_LED; bit++) {
      sendPulse(color & (0x800000 >> bit));
    }
  }
  latchAllChannels(false);
  interrupts();

  delayMicroseconds(300);
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(DIN, OUTPUT);
  pinMode(WCK, OUTPUT);
  pinMode(BCK, OUTPUT);

  digitalWrite(DIN, LOW);
  digitalWrite(WCK, LOW);
  digitalWrite(BCK, LOW);

  Serial.begin(115200);
}

void loop() {
  sendPl9823Frame();
  frame++;

  digitalWrite(LED_BUILTIN, HIGH);
  Serial.println("PL9823 bit-bang tower test: ON");
  delay(250);

  sendPl9823Frame();
  frame++;

  digitalWrite(LED_BUILTIN, LOW);
  Serial.println("PL9823 bit-bang tower test: OFF");
  delay(250);
}
