#include <Arduino.h>
#include <SPI.h>
#include <TFT_eSPI.h>

#include "PinConfig.h"

namespace {
TFT_eSPI display;
SPIClass touchSpi(HSPI);

constexpr int SCREEN_WIDTH = 320;
constexpr int SCREEN_HEIGHT = 240;
constexpr int RAW_MIN = 250;
constexpr int RAW_MAX = 3850;

uint16_t background = TFT_BLACK;
uint32_t touchCount = 0;

void drawButton(int x, uint16_t color, const char* label) {
  display.fillRect(x, 184, 100, 48, color);
  display.drawRect(x, 184, 100, 48, TFT_WHITE);
  display.setTextDatum(MC_DATUM);
  display.setTextColor(color == TFT_WHITE ? TFT_BLACK : TFT_WHITE, color);
  display.drawString(label, x + 50, 208, 2);
}

void drawScreen() {
  display.fillScreen(background);
  display.setTextDatum(TL_DATUM);
  display.setTextColor(TFT_WHITE, background);
  display.drawString("LCD + TOUCH TEST", 12, 10, 4);
  display.drawFastHLine(12, 43, 296, TFT_YELLOW);

  display.fillRect(12, 56, 70, 38, TFT_RED);
  display.fillRect(87, 56, 70, 38, TFT_GREEN);
  display.fillRect(162, 56, 70, 38, TFT_BLUE);
  display.fillRect(237, 56, 70, 38, TFT_WHITE);

  display.setTextColor(TFT_CYAN, background);
  display.drawString("Touch the screen", 12, 110, 2);
  display.setTextColor(TFT_LIGHTGREY, background);
  display.drawString("Raw: ----, ----", 12, 136, 2);
  display.drawString("Touch count: 0", 12, 158, 2);

  drawButton(8, TFT_RED, "RED");
  drawButton(110, TFT_GREEN, "GREEN");
  drawButton(212, TFT_BLUE, "BLUE");
}

void showTouch(int rawX, int rawY) {
  int x = constrain(map(rawX, RAW_MIN, RAW_MAX, 0, SCREEN_WIDTH - 1),
                    0, SCREEN_WIDTH - 1);
  int y = constrain(map(rawY, RAW_MIN, RAW_MAX, 0, SCREEN_HEIGHT - 1),
                    0, SCREEN_HEIGHT - 1);

  ++touchCount;
  Serial.printf("Touch raw x=%d y=%d -> x=%d y=%d\n", rawX, rawY, x, y);

  display.fillRect(12, 132, 296, 45, background);
  display.setTextDatum(TL_DATUM);
  display.setTextColor(TFT_LIGHTGREY, background);
  display.drawString("Raw: " + String(rawX) + ", " + String(rawY),
                     12, 136, 2);
  display.drawString("Touch count: " + String(touchCount), 12, 158, 2);

  display.drawCircle(x, y, 7, TFT_YELLOW);
  display.drawFastHLine(x - 10, y, 21, TFT_YELLOW);
  display.drawFastVLine(x, y - 10, 21, TFT_YELLOW);

  if (y >= 180) {
    if (x < 106) background = TFT_RED;
    else if (x < 210) background = TFT_GREEN;
    else background = TFT_BLUE;
    drawScreen();
  }
}

int bestTwoAverage(int a, int b, int c) {
  int ab = abs(a - b);
  int ac = abs(a - c);
  int bc = abs(b - c);
  if (ab <= ac && ab <= bc) return (a + b) / 2;
  if (ac <= ab && ac <= bc) return (a + c) / 2;
  return (b + c) / 2;
}

void armTouchIrq() {
  touchSpi.beginTransaction(SPISettings(2000000, MSBFIRST, SPI_MODE0));
  digitalWrite(Pins::TOUCH_CS, LOW);
  touchSpi.transfer(0xD0);      // Power-down mode enables PENIRQ.
  touchSpi.transfer16(0);
  digitalWrite(Pins::TOUCH_CS, HIGH);
  touchSpi.endTransaction();
}

void readTouchPoint(int& x, int& y) {
  int data[6];
  touchSpi.beginTransaction(SPISettings(2000000, MSBFIRST, SPI_MODE0));
  digitalWrite(Pins::TOUCH_CS, LOW);

  // The XPT2046 returns each conversion while the next command is shifted in.
  touchSpi.transfer(0xB1);
  touchSpi.transfer16(0xC1);
  touchSpi.transfer16(0x91);
  touchSpi.transfer16(0x91);  // First X conversion is intentionally discarded.
  data[0] = touchSpi.transfer16(0xD1) >> 3;
  data[1] = touchSpi.transfer16(0x91) >> 3;
  data[2] = touchSpi.transfer16(0xD1) >> 3;
  data[3] = touchSpi.transfer16(0x91) >> 3;
  data[4] = touchSpi.transfer16(0xD0) >> 3;
  data[5] = touchSpi.transfer16(0) >> 3;

  digitalWrite(Pins::TOUCH_CS, HIGH);
  touchSpi.endTransaction();
  x = bestTwoAverage(data[0], data[2], data[4]);
  y = bestTwoAverage(data[1], data[3], data[5]);
}
}  // namespace

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println("LCD + Touch test starting");

  pinMode(Pins::TFT_BACKLIGHT, OUTPUT);
  digitalWrite(Pins::TFT_BACKLIGHT, HIGH);
  pinMode(Pins::SD_CS, OUTPUT);
  digitalWrite(Pins::SD_CS, HIGH);
  pinMode(Pins::SCREEN_CS, OUTPUT);
  digitalWrite(Pins::SCREEN_CS, HIGH);
  pinMode(Pins::TOUCH_CS, OUTPUT);
  digitalWrite(Pins::TOUCH_CS, HIGH);
  pinMode(Pins::TOUCH_IRQ, INPUT);

  SPI.begin(Pins::SPI_SCK, Pins::SPI_MISO, Pins::SPI_MOSI);
  touchSpi.begin(Pins::TOUCH_SCK, Pins::TOUCH_MISO,
                 Pins::TOUCH_MOSI, Pins::TOUCH_CS);

  display.init();
  display.setRotation(1);
  drawScreen();
  Serial.println("LCD ready");

  armTouchIrq();
  Serial.println("Touch ready; waiting for active-low IRQ");
}

void loop() {
  static uint32_t lastSample = 0;
  static uint32_t lastReport = 0;
  static uint8_t stableFrames = 0;
  if (millis() - lastSample < 35) return;
  lastSample = millis();

  int minX = 4095;
  int maxX = 0;
  int minY = 4095;
  int maxY = 0;
  int32_t xTotal = 0;
  int32_t yTotal = 0;
  for (int sample = 0; sample < 4; ++sample) {
    int x;
    int y;
    readTouchPoint(x, y);
    minX = min(minX, x);
    maxX = max(maxX, x);
    minY = min(minY, y);
    maxY = max(maxY, y);
    xTotal += x;
    yTotal += y;
  }

  int x = xTotal / 4;
  int y = yTotal / 4;
  bool valid = x > 150 && x < 3950 && y > 150 && y < 3950;
  bool stable = maxX - minX < 70 && maxY - minY < 70;
  stableFrames = valid && stable ? min<int>(stableFrames + 1, 3) : 0;

  if (millis() - lastReport >= 500) {
    lastReport = millis();
    Serial.printf("Touch x=%d y=%d spread=%d,%d stable=%s\n",
                  x, y, maxX - minX, maxY - minY,
                  stable ? "yes" : "no");
  }

  if (stableFrames >= 2) showTouch(x, y);
}
