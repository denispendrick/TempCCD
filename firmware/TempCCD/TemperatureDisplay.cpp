#include "TemperatureDisplay.h"

#include <Wire.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

namespace {
constexpr char kTitle[] = "Pyrometer";
constexpr uint8_t kDegree = 0xDF;  // degree glyph in the HD44780 character ROM
}  // namespace

bool TemperatureDisplay::begin() {
  Wire.begin();
  Wire.beginTransmission(LCD_I2C_ADDRESS);
  if (Wire.endTransmission() != 0) {
    return false;  // nobody home at this address
  }

  lcd_.init();
  lcd_.backlight();
  lcd_.clear();
  return true;
}

void TemperatureDisplay::showSplash() {
  printLine(0, kTitle);
  printLine(1, "Initializing...");
}

void TemperatureDisplay::showTemperature(float celsius) {
  if (isnan(celsius)) {
    showStatus("NO TARGET");
    return;
  }

  char line[LCD_COLUMNS + 1];
  snprintf(line, sizeof(line), "Temp:%6.1f%cC", celsius, kDegree);
  printLine(0, kTitle);
  printLine(1, line);
}

void TemperatureDisplay::showStatus(const char* message) {
  printLine(0, kTitle);
  printLine(1, message);
}

// Write exactly LCD_COLUMNS characters so leftover text from a longer previous
// line never lingers.
void TemperatureDisplay::printLine(uint8_t row, const char* text) {
  char buf[LCD_COLUMNS + 1];
  const size_t len = strlen(text);
  for (uint8_t i = 0; i < LCD_COLUMNS; ++i) {
    buf[i] = (i < len) ? text[i] : ' ';
  }
  buf[LCD_COLUMNS] = '\0';

  lcd_.setCursor(0, row);
  lcd_.print(buf);
}
