// ---- OLED display helpers -------------------------------------------------
// Handles rendering to the SSD1306 display. See JxPunchKit.ino
// for the OledTextData struct definition and shared `display` object.

// Renders a fully-configured message (custom font sizes/colors per line).
void oledPrintMessage(OledTextData &textData) {
  display.clearDisplay();
  display.setCursor(0, 0);

  display.setTextSize(textData.line1Large ? 2 : 1);
  display.setTextColor(textData.line1Color);
  display.println(textData.line1Text);

  if (textData.line2Text != "") {
    display.setTextSize(textData.line2Large ? 2 : 1);
    display.setTextColor(textData.line2Color);
    display.println(textData.line2Text);
  }

  if (textData.line3Text != "") {
    display.setTextSize(textData.line3Large ? 2 : 1);
    display.setTextColor(textData.line3Color);
    display.println(textData.line3Text);
  }

  display.display();
  delay(OLED_DELAY_MS);  // pause so the message is readable before next update
}

// Convenience wrapper for simple two-line white messages (the common case).
// For custom font size/color, build an OledTextData and call
// oledPrintMessage() directly instead.
void showMessage(String line1, String line2) {
  oledTextData = { line1, line2, "", true, true, false, WHITE, WHITE, WHITE };
  oledPrintMessage(oledTextData);
}