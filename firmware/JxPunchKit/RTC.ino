// ---- RTC helpers -------------------------------------------------
// The DS3231 always stores UTC internally. UTC_OFFSET_HOURS (defined in
// JxPunchKit.ino) converts between stored UTC and local wall-clock time.
// This isolates all DST/timezone-law uncertainty to one constant.
void rtcInit() {
  if (!rtc.begin()) {
    Serial.println("RTC not found! Check wiring");
    showMessage("RTC ERROR", "Check wiring");
    while (true);  // halt on error
  }

  // if (rtc.lostPower()) {
  // ONE-TIME RESYNC!!
  if (true) {
  // if (rtc.lostPower()) {
    Serial.println("RTC lost power. Setting to compile time (converted to UTC).");
    DateTime compileLocal = DateTime(F(__DATE__), F(__TIME__));
    DateTime compileUTC = compileLocal - TimeSpan(0, UTC_OFFSET_HOURS, 0, 0);
    rtc.adjust(compileUTC);
  }
}

// Returns current local time by applying UTC_OFFSET_HOURS to the RTC's
// stored UTC time. Use this everywhere "now" is needed for display/logging.
DateTime getLocalTime() {
  DateTime utcNow = rtc.now();
  return utcNow + TimeSpan(0, UTC_OFFSET_HOURS, 0, 0);
}

// Formats a DateTime as HH:MM:SS, zero-padded
String getTimestamp(DateTime dt) {
  char buf[9];
  sprintf(buf, "%02d:%02d:%02d", dt.hour(), dt.minute(), dt.second());
  return String(buf);
}