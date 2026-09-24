#ifndef PIN_CONFIG_H
#define PIN_CONFIG_H

namespace Pins {
constexpr int SCREEN_CS = 27;
constexpr int SCREEN_DC = 26;
constexpr int SCREEN_RESET = 25;
constexpr int TFT_BACKLIGHT = 33;

constexpr int TOUCH_CS = 14;
constexpr int TOUCH_IRQ = 34;
constexpr int SD_CS = 13;

constexpr int SPI_SCK = 18;
constexpr int SPI_MISO = 19;
constexpr int SPI_MOSI = 23;

constexpr int TEENSY_RX = 16;
constexpr int TEENSY_TX = 17;
}  // namespace Pins

#endif
