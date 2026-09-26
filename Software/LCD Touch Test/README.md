# LCD + Touch Test

Standalone ESP32 test for the 2.4-inch ILI9341 SPI display and XPT2046
resistive touch controller. It does not depend on the Teensy or LED project.

## Wiring

| LCD pin | ESP32 pin | Purpose |
| --- | ---: | --- |
| VCC | 3V3 | Power |
| GND | GND | Ground |
| CS | GPIO 27 | Display select |
| RESET | GPIO 25 | Display reset |
| DC | GPIO 26 | Display data/command |
| SDI (MOSI) | GPIO 23 | Shared SPI MOSI |
| SCK | GPIO 18 | Shared SPI clock |
| LED | GPIO 33 | Backlight |
| SDO (MISO) | GPIO 19 | Shared SPI MISO |
| T_CLK | GPIO 4 | Touch SPI clock |
| T_CS | GPIO 21 | Touch select |
| T_DIN | GPIO 16 | Touch SPI MOSI |
| T_DO | GPIO 17 | Touch SPI MISO |
| T_IRQ | GPIO 34 | Touch interrupt |

Leave the SD-card pins disconnected for this test. All grounds must be common.
Do not feed 5 V into an ESP32 GPIO.

## Expected result

The display shows a title, four color blocks, touch coordinates, and three large
buttons. Touching the screen draws a yellow cross and prints raw coordinates at
115200 baud. Pressing RED, GREEN, or BLUE changes the background color.

The touch coordinates use approximate calibration values. Mirroring or an
offset is acceptable in this wiring test and will be calibrated later.

## Build and upload

```sh
/home/alon/.platformio/penv/bin/pio run
/home/alon/.platformio/penv/bin/pio run -t upload --upload-port /dev/ttyUSB0
```
