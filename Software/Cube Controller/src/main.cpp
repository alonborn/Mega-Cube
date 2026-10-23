#include <Arduino.h>
#include <NimBLEDevice.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include <Preferences.h>

#include "AnimationCatalog.h"
#include "PinConfig.h"
#include "Protocol.h"

namespace {
TFT_eSPI display;
XPT2046_Touchscreen touch(Pins::TOUCH_CS, Pins::TOUCH_IRQ);
HardwareSerial teensy(2);
struct ControllerCommand { char text[181]; uint8_t size; };
AnimationCatalog animationCatalog;
Preferences catalogStorage;
bool catalogStorageReady = false;
QueueHandle_t commandQueue = nullptr;
portMUX_TYPE musicMux = portMUX_INITIALIZER_UNLOCKED;
uint8_t latestMusic[12] = {};
bool musicPending = false;

NimBLECharacteristic* statusCharacteristic = nullptr;
volatile bool phoneConnected = false;
volatile bool uiDirty = true;
uint8_t animationId = 22;
bool playlistActive = false;
bool playbackConfirmed = false;
String teensyLine;
constexpr bool ENABLE_DISPLAY = true;
constexpr bool ENABLE_TOUCH = true;

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
  const char* name = animationCatalog.name(id);
  return name[0] ? name : "UNKNOWN";
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
    portENTER_CRITICAL(&musicMux);
    musicPending = false;
    portEXIT_CRITICAL(&musicMux);
    Serial.printf("BLE disconnected: %d\n", reason);
    uiDirty = true;
  }
};

class CommandCallbacks : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic* characteristic,
               NimBLEConnInfo&) override {
    std::string value = characteristic->getValue();
    if (value.empty()) return;
    if (value.size() == 16 && value[0] == 'M' && value[1] == 'U') {
      uint8_t checksum = 0;
      for (char byte : value) checksum ^= static_cast<uint8_t>(byte);
      if (static_cast<uint8_t>(value[2]) != 1 || checksum != 0) return;
      portENTER_CRITICAL(&musicMux);
      memcpy(latestMusic, value.data() + 3, sizeof(latestMusic));
      musicPending = true;
      portEXIT_CRITICAL(&musicMux);
      return;
    }
    if (value.size() > 180) return;
    if (value.size() >= 2 && value[0] == 'A' && value[1] == 'C') {
      ControllerCommand queued = {};
      queued.size = value.size();
      memcpy(queued.text, value.data(), value.size());
      if (!commandQueue || xQueueSend(commandQueue, &queued, 0) != pdTRUE)
        notifyStatus("ERROR CATALOG");
      return;
    }
    String command(value.c_str());
    command.trim();
    Serial.printf("BLE command: %s\n", command.c_str());
    ControllerCommand queued = {};
    command.toCharArray(queued.text, sizeof(queued.text));
    if (!commandQueue || xQueueSend(commandQueue, &queued, 0) != pdTRUE)
      notifyStatus("ERROR COMMAND_QUEUE_FULL");

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
  const char* playing = playbackConfirmed
                         ? (playlistActive ? "PLAYLIST"
                                           : animationName(animationId))
                         : "WAITING FOR TEENSY";
  const int font = display.textWidth(playing, 4) <= 304 ? 4
                   : display.textWidth(playing, 2) <= 304 ? 2 : 1;
  display.drawString(playing, 160, 202, font);

  display.setTextDatum(BC_DATUM);
  display.setTextColor(TFT_DARKGREY, COLOR_BACKGROUND);
  display.drawString("BLE: Mega Cube", 160, 236, 2);
}

void handleTouch() {
  static uint32_t lastTouch = 0;
  if (!touch.touched() || millis() - lastTouch < 250) return;
  lastTouch = millis();

  TS_Point point = touch.getPoint();
  int x = map(point.x, 250, 3850, 320, 0);
  int y = map(point.y, 250, 3850, 0, 240);
  x = constrain(x, 0, 319);
  y = constrain(y, 0, 239);

  if (y < 180 || y > 232) return;
  if (x < 108) {
    selectAnimation(animationCatalog.adjacent(animationId, -1));
  } else if (x < 208) {
    selectAnimation(animationCatalog.adjacent(animationId, 1));
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

void forwardCommands() {
  ControllerCommand command;
  for (uint8_t i = 0; i < 2 && commandQueue; ++i) {
    if (xQueueReceive(commandQueue, &command, 0) != pdTRUE) break;
    if (command.size) {
      uint8_t id = 0;
      bool changed = false;
      const auto result = animationCatalog.accept(
          reinterpret_cast<const uint8_t*>(command.text), command.size, id, changed);
      if (result == AnimationCatalog::INVALID) notifyStatus("ERROR CATALOG");
      if (result == AnimationCatalog::COMPLETE) {
        bool saved = true;
        if (id >= AnimationCatalog::DEFAULT_COUNT) {
          const String key = "a" + String(id);
          saved = catalogStorageReady;
          if (saved && catalogStorage.getString(key.c_str()) != animationCatalog.name(id))
            saved = catalogStorage.putString(key.c_str(), animationCatalog.name(id)) > 0;
        }
        if (changed) uiDirty = true;
        Serial.printf("Catalog ID=%u changed=%d saved=%d\n", id, changed, saved);
        notifyStatus(saved ? "OK CATALOG " + String(id) : "ERROR CATALOG_SAVE");
      }
      continue;
    }
    if (strcmp(command.text, "CATALOG?") == 0) {
      notifyStatus("CATALOG 1 " + String(AnimationCatalog::DEFAULT_COUNT));
      continue;
    }
    sendToTeensy(String(command.text));
  }
}

void forwardMusic() {
  if (teensy.availableForWrite() < 31) return;
  uint8_t data[12];
  portENTER_CRITICAL(&musicMux);
  const bool pending = musicPending;
  if (pending) {
    memcpy(data, latestMusic, sizeof(data));
    musicPending = false;
  }
  portEXIT_CRITICAL(&musicMux);
  if (!pending) return;

  // Fixed-size line fits the existing UART parser without logging every frame.
  const char hex[] = "0123456789ABCDEF";
  char line[31];
  memcpy(line, "MUSIC ", 6);
  for (uint8_t i = 0; i < 12; ++i) {
    line[6 + i * 2] = hex[data[i] >> 4];
    line[7 + i * 2] = hex[data[i] & 15];
  }
  line[30] = '\n';
  teensy.write(reinterpret_cast<const uint8_t*>(line), sizeof(line));
}
}  // namespace

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("Mega Cube controller starting");
  catalogStorageReady = catalogStorage.begin("animations", false);
  if (catalogStorageReady) {
    for (uint16_t id = AnimationCatalog::DEFAULT_COUNT; id < 256; ++id) {
      const String key = "a" + String(id);
      if (catalogStorage.isKey(key.c_str()))
        animationCatalog.restore(id, catalogStorage.getString(key.c_str()).c_str());
    }
  }
  commandQueue = xQueueCreate(12, sizeof(ControllerCommand));
  teensy.begin(115200, SERIAL_8N1, Pins::TEENSY_RX, Pins::TEENSY_TX);

  if (ENABLE_DISPLAY) {
    pinMode(Pins::TFT_BACKLIGHT, OUTPUT);
    digitalWrite(Pins::TFT_BACKLIGHT, HIGH);
    pinMode(Pins::SD_CS, OUTPUT);
    digitalWrite(Pins::SD_CS, HIGH);

    SPI.begin(Pins::SPI_SCK, Pins::SPI_MISO, Pins::SPI_MOSI);
    if (ENABLE_TOUCH) touch.begin(SPI);
    display.init();
    display.setRotation(1);
    drawUi();
  }

  setupBle();
  sendToTeensy("STATUS");
}

void loop() {
  forwardCommands();
  forwardMusic();
  if (ENABLE_TOUCH) handleTouch();
  forwardTeensyStatus();
  if (ENABLE_DISPLAY && uiDirty) drawUi();
  delay(5);
}
