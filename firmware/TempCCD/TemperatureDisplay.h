#pragma once

#include <Arduino.h>
#include <LiquidCrystal_I2C.h>

#include "Config.h"

// ============================================================================
//  TemperatureDisplay - thin wrapper around a 16x2 I2C character LCD
// ----------------------------------------------------------------------------
//  Keeps the formatting and padding in one place so the main loop only has to
//  hand over a temperature or a short status string.
// ============================================================================
class TemperatureDisplay {
 public:
  // Probes the I2C bus for the LCD and initialises it. Returns false if nothing
  // answers at LCD_I2C_ADDRESS, so the caller can fall back to Serial.
  bool begin();

  void showSplash();
  void showTemperature(float celsius);  // NAN renders as a "no target" status
  void showStatus(const char* message);

 private:
  LiquidCrystal_I2C lcd_{LCD_I2C_ADDRESS, LCD_COLUMNS, LCD_ROWS};
  void printLine(uint8_t row, const char* text);
};
