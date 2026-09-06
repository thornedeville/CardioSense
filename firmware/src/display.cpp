#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "display.h"

LiquidCrystal_I2C lcd(0x27, 16, 2);

void displayInit() {
  lcd.init();
  lcd.backlight();
  lcd.clear();
}

void showStatus(const char* status) {
  lcd.setCursor(0, 0);
  lcd.print("Status: ");
  lcd.print(status);
}

void showHeartRate(int bpm) {
  lcd.setCursor(0, 1);
  lcd.print("HR: ");
  lcd.print(bpm);
  lcd.print(" bpm  "); // trailing spaces clear old digits
}

void showAnomaly(bool isAnomaly, const char* label) {
  char temp[17];
  if (isAnomaly) {
    snprintf(temp, sizeof(temp), "ALERT: %s", label);
  } else {
    snprintf(temp, sizeof(temp), "Status: NORMAL");
  }

  char buf[17];
  snprintf(buf, sizeof(buf), "%-16s", temp); // pad/truncate to exactly 16 chars

  lcd.setCursor(0, 0);
  lcd.print(buf);
}