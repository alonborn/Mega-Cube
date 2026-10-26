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
| ESP32 RX2 from Teensy TX1 | 16 |
| ESP32 TX2 to Teensy RX1 | 17 |

Connect ESP32 and Teensy grounds. Both boards use 3.3 V UART logic.
UART2 is independent of the ESP32 USB-to-serial bridge, so the controller can
remain connected to the Teensy while flashing or monitoring over USB.

## Commands

BLE and UART use newline-terminated UTF-8 commands:

```text
ANIMATION 22
PLAYLIST
BRIGHTNESS 180
STATUS
```

Build with `/home/alon/.platformio/penv/bin/pio run`.

## Dynamic animation catalog

The built-in IDs 0..39 remain in `include/AnimationCatalog.h`. Adding an animation
now requires updating only the Teensy implementation and Android's
`AnimationCatalog.choices`. Append new IDs (40..255); do not renumber existing IDs.
After a one-time ESP32 firmware upgrade, the phone synchronizes names on every
BLE connection. The ESP32 only forwards animation commands to the Teensy; no
animation implementation is downloaded to the ESP32.

Additional names are stored in the `animations` Preferences/NVS namespace and
survive restarts and ordinary firmware uploads. Unchanged entries are not written
again. A partial transfer never replaces a name. Built-in names cannot be
overridden, and reconnecting with an older phone does not remove learned entries.
Explicit deletion records from an updated phone remove a retired learned entry
from RAM and NVS; omitted entries alone are never treated as deletions.
Touch previous/next cycles through known IDs, skipping gaps, including learned
entries. Playback names still appear only after Teensy confirmation.

To promote animations to defaults, extend the contiguous default-name array and
`DEFAULT_COUNT`, then flash the ESP32 when desired. Its advertised default count
causes the phone to skip these entries automatically.

Protocol (handled locally, never sent to the Teensy):

- Phone sends `CATALOG?`; ESP32 responds `CATALOG 1 <default-count>`.
- For each additional ID, phone sends binary fragments: `A C`, version 1, ID,
  byte offset, total name length, then up to 14 ASCII bytes. Every fragment fits
  the 20-byte default BLE payload. Names must contain 1..32 printable ASCII bytes.
- ESP32 assembles fragments in order, persists a complete name, then notifies
  `OK CATALOG <id>` (or `ERROR CATALOG` / `ERROR CATALOG_SAVE`). The phone waits
  for this confirmation before transferring another entry. Failed transfers
  are retried on reconnect; normal controls remain usable.
- A six-byte `A C 1 <id> 0 0` packet deletes a learned entry, using the same
  confirmation. Default entries are protected. Repeated deletion is harmless.

Native parser/navigation test:

```bash
g++ -std=c++11 -Iinclude test/catalog_test.cpp -o /tmp/cube-catalog-test
/tmp/cube-catalog-test
```
