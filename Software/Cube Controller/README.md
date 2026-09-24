# Cube Controller

ESP32-WROOM-32 controller for the Mega Cube. It provides BLE control, a local
ILI9341 display with XPT2046 touch, and a UART bridge to the Teensy LED driver.

## Wiring

| Signal | ESP32 GPIO |
|---|---:|
| TFT SCK / Touch CLK / SD SCK | 18 |
| TFT MISO / Touch DO / SD MISO | 19 |
| TFT MOSI / Touch DIN / SD MOSI | 23 |
| TFT CS | 27 |
| TFT DC | 26 |
| TFT RESET | 25 |
| TFT LED | 33 |
| Touch CS | 14 |
| Touch IRQ | 34 |
| SD CS | 13 |
| ESP32 RX from Teensy TX1 | 16 |
| ESP32 TX to Teensy RX1 | 17 |

Connect ESP32 and Teensy grounds. Both boards use 3.3 V UART logic.

## Commands

BLE and UART use newline-terminated UTF-8 commands:

```text
ANIMATION 22
PLAYLIST
BRIGHTNESS 180
STATUS
```

Build with `/home/alon/.platformio/penv/bin/pio run`.

