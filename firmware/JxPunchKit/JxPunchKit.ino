/*
 * Project:     JxPunchKit
 * Description: Custom RFID-based employee timeclock. Scans RFID fobs/cards
 *              to clock employees in/out, with OLED + buzzer feedback.
 *              Local storage and Google Sheets sync planned.
 *
 * Author:      Justin (https://github.com/Jnxvln?tab=repositories)
 * Created:     July 2026
 * Board:       ESP32 Dev Module (ESP32 DevKit V1)
 *
 * Libraries:
 *   - MFRC522 (GithubCommunity)
 *   - Adafruit GFX Library
 *   - Adafruit SSD1306
 *
 * Hardware:
 *   - RC522 RFID reader   (SPI)
 *   - SSD1306 OLED 128x64 (I2C, addr 0x3C)
 *   - DS3231 RTC          (I2C, addr 0x68)
 *   - KY-006 passive piezo buzzer
 *
 * Pinout:
 *   RC522   SDA=5  SCK=18  MOSI=23  MISO=19  RST=22
 *   OLED    SDA=32 SCL=21
 *   RTC     SDA=32 SCL=21  (shared I2C bus with OLED)
 *   Buzzer  GPIO 4
 */

#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <RTClib.h>

// ---- Pin assignments -------------------------------------------------
#define I2C_SDA 32
#define I2C_SCL 21

#define SS_PIN 5
#define RST_PIN 22
#define BUZZER_PIN 4

// ---- OLED config -------------------------------------------------
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDRESS 0x3C
#define WHITE SSD1306_WHITE

// ---- Timeclock config -------------------------------------------------
#define NUM_EMPLOYEES 3
#define OLED_DELAY_MS 2000
// UTC offset for local time display, in hours. Update this whenever DST
// changes or applicable time law changes. Nothing else needs to change.
// Central Daylight Time (CDT, in effect roughly Mar-Nov) = -5
// Central Standard Time (CST, in effect roughly Nov-Mar) = -6
#define UTC_OFFSET_HOURS -5

// Admin card UID, broken into individual bytes for direct comparison
#define ADMIN_UID_0 0xB4
#define ADMIN_UID_1 0x30
#define ADMIN_UID_2 0xCA
#define ADMIN_UID_3 0x06

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
MFRC522 rfid(SS_PIN, RST_PIN);
RTC_DS3231 rtc;

// Returns true if the scanned UID matches the admin card
bool isAdminCard(byte *scannedUid, byte uidLength) {
  if (uidLength != 4) return false;
  return (scannedUid[0] == ADMIN_UID_0 &&
          scannedUid[1] == ADMIN_UID_1 &&
          scannedUid[2] == ADMIN_UID_2 &&
          scannedUid[3] == ADMIN_UID_3);
}

struct Employee {
  byte uid[4];       // 4-byte UID read from the fob
  String name;       // Employee display name
  bool isClockedIn;  // true = currently clocked in, false = currently out
};

struct OledTextData {
  String line1Text;     // Line 1 text
  String line2Text;     // Line 2 text
  String line3Text;     // Line 3 text
  bool line1Large;      // Line 1 large font size
  bool line2Large;      // Line 2 large font size
  bool line3Large;      // Line 3 large font size
  uint16_t line1Color;  // Line 1 text color
  uint16_t line2Color;  // Line 2 text color
  uint16_t line3Color;  // Line 3 text color
};

// NOTE: hardcoded for now, will move to persistent storage once
// the RTC/storage work is in place, so employees can be added
// without reflashing firmware.
Employee employees[NUM_EMPLOYEES] = {
  {{0xA9, 0x5D, 0x4E, 0x06}, "John", false},
  {{0xC2, 0x4A, 0x10, 0x7E}, "Tina", false},
  {{0xB4, 0x30, 0xCA, 0x06}, "ADMIN", false}
};

OledTextData oledTextData = { "", "", "", true, true, false, WHITE, WHITE, WHITE };

char buffer[50];


// ==== Function prototypes ===================================================
// Manual prototypes are required for functions with default parameters. 
// Arduino's auto-prototype generator doesn't handle those reliably.

void oledPrintMessage(OledTextData &textData);
void showMessage(String line1, String line2 = "");
void rtcInit();
DateTime getLocalTime();
String getTimestamp(DateTime dt);

// ============================================================================


void setup() {
  Serial.begin(115200);
  delay(1000);

  SPI.begin();       // starts SPI bus (default VSPI pins: 18, 19, 23)
  rfid.PCD_Init();   // initialize the RC522 reader chip
  delay(100);

  // Initialize OLED
  Wire.begin(I2C_SDA, I2C_SCL);  // shared I2C bus for OLED + RTC
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
    Serial.println("OLED not found! Check wiring/address.");
    while (true); // halt here, nothing useful can happen without the display
  }
  Serial.println("OLED initialized");

  // Initialize the RTC module
  rtcInit();

  // Boot sequence messages
  Serial.println("PunchKit");
  showMessage("PunchKit");
  delay(1000);

  Serial.println("Checking RC522 connection...");
  showMessage("Checking", "Reader..");

  byte version = rfid.PCD_ReadRegister(rfid.VersionReg);
  Serial.print("RC522 firmware version: 0x");
  Serial.println(version, HEX);
  Serial.println();

  Serial.println("RFID Scanner ready!");
  //oledTextData = { "SCANNER", "READY!", true, true, WHITE, SSD1306_INVERSE };
  showMessage("SCANNER", "READY!");
}

// ============================================================================

void loop() {
  DateTime now = getLocalTime();

  if (!rfid.PICC_IsNewCardPresent()) {
    return;
  }

  if (!rfid.PICC_ReadCardSerial()) {
    return;
  }

  // Log the scanned UID as hex, useful for registering new fobs
  Serial.print("\nCard UID: ");
  for (byte i = 0; i < rfid.uid.size; i++) {
    Serial.print(rfid.uid.uidByte[i] < 0x10 ? " 0" : " ");
    Serial.print(rfid.uid.uidByte[i], HEX);
  }
  Serial.println();

  if (isAdminCard(rfid.uid.uidByte, rfid.uid.size)) {
    Serial.println("Admin card detected.");

    tone(BUZZER_PIN, 2000);
    delay(150);
    noTone(BUZZER_PIN);

    showMessage("ADMIN", "DETECTED");
  } else {
    int employeeIndex = findEmployee(rfid.uid.uidByte, rfid.uid.size);

    if (employeeIndex == -1) {
      // Unrecognized fob: three quick beeps
      for (int i = 0; i < 3; i++) {
        tone(BUZZER_PIN, 900);
        delay(100);
        noTone(BUZZER_PIN);
        delay(100);
      }

      Serial.println("-Unknown Device-");
      showMessage("Unknown", "Device!");
    } else {
      // Known employee: flip their clocked-in status
      employees[employeeIndex].isClockedIn = !employees[employeeIndex].isClockedIn;

      tone(BUZZER_PIN, 1000);
      delay(150);
      noTone(BUZZER_PIN);

      if (employees[employeeIndex].isClockedIn) {
        Serial.print(employees[employeeIndex].name);
        sprintf(buffer, " clocked IN (%02d:%02d:%02d)", now.hour(), now.minute(), now.second());
        Serial.println(buffer);

        oledTextData = { employees[employeeIndex].name, "-IN-", getTimestamp(now), true, true, false, WHITE, WHITE, WHITE };
        oledPrintMessage(oledTextData);
      } else {
        Serial.print(employees[employeeIndex].name);
        sprintf(buffer, " clocked OUT (%02d:%02d:%02d)", now.hour(), now.minute(), now.second());
        Serial.println(buffer);

        oledTextData = { employees[employeeIndex].name, "-OUT-", getTimestamp(now), true, true, false, WHITE, WHITE, WHITE };
        oledPrintMessage(oledTextData);
      }
    }
  }

  rfid.PICC_HaltA();  // let this card be freshly re-detected next time
  delay(1000);        // basic debounce, will move to millis()-based timing later
}

// Returns the index of the employee matching scannedUid, or -1 if not found
int findEmployee(byte *scannedUid, byte uidLength) {
  if (uidLength != 4) return -1;

  for (int i = 0; i < NUM_EMPLOYEES; i++) {
    bool match = true;
    for (int j = 0; j < 4; j++) {
      if (employees[i].uid[j] != scannedUid[j]) {
        match = false;
        break;
      }
    }
    if (match) return i;
  }
  return -1;
}