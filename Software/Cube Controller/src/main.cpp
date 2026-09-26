#include <Arduino.h>
#include <NimBLEDevice.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>

#include "PinConfig.h"
#include "Protocol.h"

namespace {
TFT_eSPI display;
XPT2046_Touchscreen touch(Pins::TOUCH_CS, Pins::TOUCH_IRQ);
HardwareSerial teensy(2);

NimBLECharacteristic* statusCharacteristic = nullptr;
volatile bool phoneConnected = false;
volatile bool uiDirty = true;
uint8_t animationId = 22;
bool playlistActive = false;
bool playbackConfirmed = false;
String teensyLine;
constexpr bool ENABLE_DISPLAY = true;
constexpr bool ENABLE_TOUCH = false;

constexpr uint16_t COLOR_BACKGROUND = TFT_BLACK;
constexpr uint16_t COLOR_PANEL = 0x18E3;
constexpr uint16_t COLOR_ACCENT = TFT_RED;
constexpr uint16_t COLOR_CONNECTED = TFT_GREEN;

void sendToTeensy(const String& command) {
  teensy.println(command);
  Serial.printf("UART2 -> Teensy: %s\n", command.c_str());
}

void notifyStatus(const String& status) {
  if (!statusCharacteristic) return;
  statusCharacteristic->setValue(status.c_str());
  if (phoneConnected) statusCharacteristic->notify();
}

void selectAnimation(uint8_t id) {
  sendToTeensy("ANIMATION " + String(id));
}

const char* animationName(uint8_t id) {
  static const char* const names[] = {
      "LED TEST",       "ACCELEROMETER", "ARROWS",         "ATOMS",
      "CUBE",           "FIREWORKS",     "HELIX",          "LIFE",
      "MARIO",          "PLASMA",        "PONG",           "SCROLLER",
      "SINUS",          "SPECTRUM",      "STARFIELD",      "FAIRY LIGHTS",
      "MULTI LIGHTS",   "CHANNEL TEST",  "CHANNEL COLORS", "THE MATRIX",
      "SPOTTED SPHERE", "SUPERNOVA",     "AURORA",         "BLACK HOLE",
      "METABALLS",      "ELECTRIC STORM", "DNA TUNNEL",    "EYE OF SAURON",
      "WHITE TEST",      "RED TEST"};
  return id < sizeof(names) / sizeof(names[0]) ? names[id] : "UNKNOWN";
}

void applyTeensyConfirmation(const String& status) {
  if (status.startsWith("OK ANIMATION ")) {
    animationId = constrain(status.substring(13).toInt(), 0, 255);
    playlistActive = false;
    playbackConfirmed = true;
    uiDirty = true;
  } else if (status == "OK PLAYLIST") {
    playlistActive = true;
    playbackConfirmed = true;
    uiDirty = true;
  } else if (status.startsWith("STATUS ANIMATION ")) {
    animationId = constrain(status.substring(17).toInt(), 0, 255);
    playlistActive = false;
    playbackConfirmed = true;
    uiDirty = true;
  } else if (status.startsWith("STATUS PLAYLIST ")) {
    animationId = constrain(status.substring(16).toInt(), 0, 255);
    playlistActive = true;
    playbackConfirmed = true;
    uiDirty = true;
  }
}

class ServerCallbacks : public NimBLEServerCallbacks {
  void onConnect(NimBLEServer*, NimBLEConnInfo& connection) override {
    phoneConnected = true;
    Serial.printf("BLE connected: %s\n",
                  connection.getAddress().toString().c_str());
    uiDirty = true;
  }

  void onDisconnect(NimBLEServer*, NimBLEConnInfo&, int reason) override {
    phoneConnected = false;
    Serial.printf("BLE disconnected: %d\n", reason);
    uiDirty = true;
  }
};

class CommandCallbacks : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic* characteristic,
               NimBLEConnInfo&) override {
    std::string value = characteristic->getValue();
    if (value.empty()) return;
    String command(value.c_str());
    command.trim();
    Serial.printf("BLE command: %s\n", command.c_str());
    sendToTeensy(command);

  }
};

ServerCallbacks serverCallbacks;
CommandCallbacks commandCallbacks;

void setupBle() {
  NimBLEDevice::init(CubeProtocol::DEVICE_NAME);
  NimBLEDevice::setPower(9);

  NimBLEServer* server = NimBLEDevice::createServer();
  server->setCallbacks(&serverCallbacks);
  server->advertiseOnDisconnect(true);

  NimBLEService* service = server->createService(CubeProtocol::SERVICE_UUID);
  NimBLECharacteristic* command = service->createCharacteristic(
      CubeProtocol::COMMAND_UUID,
      NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR);
  command->setCallbacks(&commandCallbacks);

  statusCharacteristic = service->createCharacteristic(
      CubeProtocol::STATUS_UUID,
      NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);
  statusCharacteristic->setValue("READY");
  server->start();

  NimBLEAdvertising* advertising = server->getAdvertising();
  advertising->addServiceUUID(CubeProtocol::SERVICE_UUID);
  advertising->enableScanResponse(true);
  advertising->setName(CubeProtocol::DEVICE_NAME);
  advertising->start();
}

void drawButton(int x, int y, int width, const char* label) {
  display.fillRoundRect(x, y, width, 52, 5, COLOR_PANEL);
  display.drawRoundRect(x, y, width, 52, 5, TFT_DARKGREY);
  display.setTextDatum(MC_DATUM);
  display.setTextColor(TFT_WHITE, COLOR_PANEL);
  display.drawString(label, x + width / 2, y + 26, 2);
}

void drawUi() {
  uiDirty = false;
  display.fillScreen(COLOR_BACKGROUND);
  display.setTextDatum(TC_DATUM);
  display.setTextColor(TFT_WHITE, COLOR_BACKGROUND);
  display.drawString("MEGA CUBE", 160, 42, 4);
  display.drawFastHLine(38, 82, 244, COLOR_ACCENT);

  display.setTextColor(phoneConnected ? COLOR_CONNECTED : TFT_ORANGE,
                       COLOR_BACKGROUND);
  display.setTextDatum(MC_DATUM);
  display.drawString(phoneConnected ? "CONNECTED" : "CONNECTING...",
                     160, 132, 4);

  display.setTextColor(TFT_DARKGREY, COLOR_BACKGROUND);
  display.setTextDatum(TC_DATUM);
  display.drawString("NOW PLAYING", 160, 174, 2);
  display.setTextColor(TFT_CYAN, COLOR_BACKGROUND);
  display.drawString(playbackConfirmed
                         ? (playlistActive ? "PLAYLIST"
                                           : animationName(animationId))
                         : "WAITING FOR TEENSY",
                     160, 202, 4);

  display.setTextDatum(BC_DATUM);
  display.setTextColor(TFT_DARKGREY, COLOR_BACKGROUND);
  display.drawString("BLE: Mega Cube", 160, 236, 2);
}

void handleTouch() {
  static uint32_t lastTouch = 0;
  if (!touch.touched() || millis() - lastTouch < 250) return;
  lastTouch = millis();

  TS_Point point = touch.getPoint();
  int x = map(point.x, 250, 3850, 0, 320);
  int y = map(point.y, 250, 3850, 0, 240);
  x = constrain(x, 0, 319);
  y = constrain(y, 0, 239);

  if (y < 180 || y > 232) return;
  if (x < 108) {
    selectAnimation(animationId == 0 ? 29 : animationId - 1);
  } else if (x < 208) {
    selectAnimation(animationId >= 29 ? 0 : animationId + 1);
  } else {
    sendToTeensy("PLAYLIST");
    notifyStatus("PLAYLIST");
  }
}

void forwardTeensyStatus() {
  while (teensy.available()) {
    char value = teensy.read();
    if (value == '\n') {
      teensyLine.trim();
      if (!teensyLine.isEmpty()) {
        Serial.printf("UART2 <- Teensy: %s\n", teensyLine.c_str());
        applyTeensyConfirmation(teensyLine);
        notifyStatus(teensyLine);
      }
      teensyLine = "";
    } else if (value != '\r' && teensyLine.length() < 180) {
      teensyLine += value;
    }
  }
}
}  // namespace

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("Mega Cube controller starting");
  teensy.begin(115200, SERIAL_8N1, Pins::TEENSY_RX, Pins::TEENSY_TX);

  if (ENABLE_DISPLAY) {
    pinMode(Pins::TFT_BACKLIGHT, OUTPUT);
    digitalWrite(Pins::TFT_BACKLIGHT, HIGH);
    pinMode(Pins::SD_CS, OUTPUT);
    digitalWrite(Pins::SD_CS, HIGH);

    SPI.begin(Pins::SPI_SCK, Pins::SPI_MISO, Pins::SPI_MOSI);
    display.init();
    display.setRotation(1);
    drawUi();
  }

  setupBle();
  sendToTeensy("STATUS");
}

void loop() {
  if (ENABLE_TOUCH) handleTouch();
  forwardTeensyStatus();
  if (ENABLE_DISPLAY && uiDirty) drawUi();
  delay(5);
}
