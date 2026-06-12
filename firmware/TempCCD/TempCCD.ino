// ============================================================================
//  TempCCD - non-contact optical pyrometer
// ----------------------------------------------------------------------------
//  An OV5640 camera on the Arduino GIGA R1 captures grayscale frames over DCMI
//  and DMA, the brightness of a hot target is read from a region of interest,
//  converted to a temperature, and shown on a 16x2 I2C LCD (and over Serial).
//
//  Pipeline:  camera -> DMA frame -> ROI brightness -> pyrometry -> LCD
//
//  Wiring and calibration live in docs/. This file is just the glue.
// ============================================================================

#include <math.h>

#include "Config.h"
#include "CameraSensor.h"
#include "Pyrometer.h"
#include "TemperatureDisplay.h"

static CameraSensor camera;
static Pyrometer pyrometer;
static TemperatureDisplay display;

static uint32_t lastRefresh = 0;
static bool haveLcd = false;

static void handleReading(const FrameStats& stats);

void setup() {
  Serial.begin(SERIAL_BAUD);

  haveLcd = display.begin();
  if (haveLcd) {
    display.showSplash();
  } else {
    Serial.println("LCD not found at configured I2C address; using Serial only.");
  }

  if (!camera.begin()) {
    Serial.println("Camera init failed - halting.");
    if (haveLcd) {
      display.showStatus("CAM INIT FAIL");
    }
    while (true) {
      delay(1000);
    }
  }

  Serial.println("TempCCD ready.");
}

void loop() {
  // Capture on the display cadence rather than spinning at full frame rate.
  const uint32_t now = millis();
  if (now - lastRefresh < DISPLAY_REFRESH_MS) {
    return;
  }
  lastRefresh = now;

  FrameStats stats;
  if (!camera.capture(stats)) {
    Serial.println("Frame capture timed out.");
    if (haveLcd) {
      display.showStatus("NO FRAME");
    }
    return;
  }

  handleReading(stats);
}

static void handleReading(const FrameStats& stats) {
  if (stats.saturated) {
    Serial.println("Saturated - stop down the optics or add a filter.");
    if (haveLcd) {
      display.showStatus("SATURATED");
    }
    return;
  }

  if (!stats.valid) {
    if (haveLcd) {
      display.showStatus("NO TARGET");
    }
    return;
  }

  const float kelvin = pyrometer.temperatureK(stats.brightness);
  if (isnan(kelvin)) {
    Serial.print("Brightness ");
    Serial.print(stats.brightness, 1);
    Serial.println(" is outside the calibrated range.");
    if (haveLcd) {
      display.showStatus("OUT OF RANGE");
    }
    return;
  }

  const float celsius = kelvin - 273.15f;
  if (haveLcd) {
    display.showTemperature(celsius);
  }

  Serial.print("dark=");
  Serial.print(stats.darkLevel, 1);
  Serial.print(" bright=");
  Serial.print(stats.brightness, 1);
  Serial.print(" peak=");
  Serial.print(stats.peak);
  Serial.print(" T=");
  Serial.print(celsius, 1);
  Serial.println(" C");
}
