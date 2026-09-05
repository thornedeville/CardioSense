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
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(isAnomaly ? "ANOMALY" : "NORMAL");
  if (isAnomaly) {
    lcd.setCursor(0, 1);
    lcd.print(label);
  }
}