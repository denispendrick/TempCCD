// ============================================================================
//  TempCCD - non-contact CCD pyrometer
// ----------------------------------------------------------------------------
//  A TCD1304 linear CCD is read through the Arduino GIGA R1's ADC + DMA, the
//  collected brightness is converted to a temperature, and the result is shown
//  on a 16x2 I2C LCD (and echoed over Serial).
//
//  Pipeline:  CCD -> DMA capture -> brightness -> pyrometry -> LCD
//
//  Wiring and calibration live in docs/. This file is just the glue.
// ============================================================================

#include <math.h>

#include "Config.h"
#include "CCDSensor.h"
#include "Pyrometer.h"
#include "TemperatureDisplay.h"

static CCDSensor ccd;
static Pyrometer pyrometer;
static TemperatureDisplay display;

static uint16_t frame[CCD_TOTAL_PIXELS];
static uint32_t lastRefresh = 0;
static bool haveLcd = false;

// Forward declarations (static functions are not auto-prototyped by the IDE).
static void handleReading(const FrameStats& stats);
#if DEBUG_DUMP_FRAME
static void dumpFrame(const uint16_t* f);
#endif

void setup() {
  Serial.begin(SERIAL_BAUD);

  haveLcd = display.begin();
  if (haveLcd) {
    display.showSplash();
  } else {
    Serial.println("LCD not found at configured I2C address; using Serial only.");
  }

  if (!ccd.begin()) {
    Serial.println("CCD/ADC init failed - halting.");
    if (haveLcd) {
      display.showStatus("CCD INIT FAIL");
    }
    while (true) {
      delay(1000);
    }
  }

  delay(50);  // let the first readouts flush through the pipeline
  Serial.println("TempCCD ready.");
}

void loop() {
  if (!ccd.frameReady() || !ccd.readFrame(frame)) {
    return;
  }

  const FrameStats stats = pyrometer.analyze(frame);

#if DEBUG_DUMP_FRAME
  dumpFrame(frame);
#endif

  const uint32_t now = millis();
  if (now - lastRefresh >= DISPLAY_REFRESH_MS) {
    lastRefresh = now;
    handleReading(stats);
  }
}

static void handleReading(const FrameStats& stats) {
  if (stats.saturated) {
    Serial.println("Saturated - reduce light or exposure.");
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

#if DEBUG_DUMP_FRAME
// Dump the whole frame over Serial at ~1 Hz for tuning the dark/signal windows.
static void dumpFrame(const uint16_t* f) {
  static uint32_t lastDump = 0;
  const uint32_t now = millis();
  if (now - lastDump < 1000) {
    return;
  }
  lastDump = now;

  for (uint16_t i = 0; i < CCD_TOTAL_PIXELS; ++i) {
    Serial.print(f[i]);
    Serial.print((i % 16 == 15) ? '\n' : ' ');
  }
  Serial.println();
}
#endif
