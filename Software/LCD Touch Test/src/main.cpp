#include <Arduino.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>

#include "PinConfig.h"

namespace {
TFT_eSPI display;
XPT2046_Touchscreen touch(Pins::TOUCH_CS, Pins::TOUCH_IRQ);

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
  int x = constrain(map(rawX, RAW_MIN, RAW_MAX, SCREEN_WIDTH - 1, 0),
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
  SPI.begin(Pins::SPI_SCK, Pins::SPI_MISO, Pins::SPI_MOSI);
  display.init();
  display.setRotation(1);
  touch.begin(SPI);
  drawScreen();
  Serial.println("LCD ready");
  Serial.println("Touch ready on shared LCD SPI bus");
}

void loop() {
  static uint32_t lastSample = 0;
  if (millis() - lastSample < 35) return;
  lastSample = millis();
  if (!touch.touched()) return;
  const TS_Point point = touch.getPoint();
  Serial.printf("Touch raw x=%d y=%d z=%d\n", point.x, point.y, point.z);
  showTouch(point.x, point.y);
}
