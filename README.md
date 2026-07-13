# JxPunchKit

A custom employee timeclock built from scratch on the ESP32. Employees badge in/out with an RFID fob or keycard. The device provides audio + on-screen feedback and (eventually) syncs punches to Google Sheets, with local storage as an offline-first fallback.

This is a learning project as much as a build, the goal for me being to understand *why* each piece works, not just ship working code.

## Status

🚧 **Actively in development.** Core scanning + display logic works! Timestamps, storage, and sync are not yet implemented.

| Feature | Status |
|---|---|
| RFID fob/card scanning | ✅ Working |
| Employee clock in/out logic | ✅ Working |
| OLED display feedback | ✅ Working |
| Buzzer audio feedback | ✅ Working |
| Real-time clock (DS3231) | 🔧 Wiring in progress |
| Non-blocking (`millis()`-based) timing | 📋 Planned |
| Local offline storage | 📋 Planned |
| Google Sheets sync | 📋 Planned |
| Hour tallying / report generation | 📋 Planned |

## Hardware

- ESP32 DevKit V1 (30-pin)
- RC522 RFID reader (SPI)
- SSD1306 OLED, 128x64 (I2C)
- DS3231 RTC module (I2C, battery-backed)
- KY-006 passive piezo buzzer
- RFID fobs + 1 admin card

## Pinout

| Component | Signal | ESP32 Pin |
|---|---|---|
| RC522 | SDA (SS) | GPIO 5 |
| RC522 | SCK | GPIO 18 |
| RC522 | MOSI | GPIO 23 |
| RC522 | MISO | GPIO 19 |
| RC522 | RST | GPIO 22 |
| OLED | SDA | GPIO 32 |
| OLED | SCL | GPIO 21 |
| RTC (DS3231) | SDA | GPIO 32 *(shared I2C bus with OLED)* |
| RTC (DS3231) | SCL | GPIO 21 *(shared I2C bus with OLED)* |
| Buzzer | Signal | GPIO 4 |

All modules run on 3.3V, matching the ESP32's logic level directly, no level shifting needed.

## Firmware structure

```
firmware/
└── JxPunchKit/
    ├── JxPunchKit.ino   # setup(), loop(), employee/admin logic
    └── OLED.ino         # display rendering helpers
```

Arduino IDE tabs are used to keep responsibilities separated (RFID/employee logic vs. display logic) while still compiling as a single sketch.

## Libraries used

- [MFRC522](https://github.com/miguelbalboa/rfid) (GithubCommunity fork)
- [Adafruit GFX Library](https://github.com/adafruit/Adafruit-GFX-Library)
- [Adafruit SSD1306](https://github.com/adafruit/Adafruit_SSD1306)
- RTClib *(planned, once RTC gets integrated)*

## Setup

1. Install the libraries above via Arduino IDE's Library Manager.
2. Select **Tools > Board > ESP32 Arduino > ESP32 Dev Module**.
3. Wire hardware per the pinout table above.
4. Open `firmware/JxPunchKit/JxPunchKit.ino` and upload.

## Roadmap

- [ ] Wire in and integrate DS3231 RTC for real punch timestamps
- [ ] Refactor blocking `delay()` calls to a `millis()`-based non-blocking pattern
- [ ] Local flash storage for offline-first punch logging
- [ ] Google Sheets sync (punch rows + hour tallying)
- [ ] Admin card actions (e.g. trigger report, enter fob-registration mode)
- [ ] Move employee list from hardcoded array to persistent storage

## License

*(License TBD once decided, e.g. MIT)*
