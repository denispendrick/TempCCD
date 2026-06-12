#pragma once

#include <Arduino.h>
#include "camera.h"

// --- OV5640 driver -----------------------------------------------------------
// The OV5640 driver is NOT in the stock GIGA Camera library (which ships Himax,
// GC2145 and OV7670). It comes from the OV5640 camera library for the GIGA
// (e.g. Arducam's). Match this include and the driver class below to whatever
// library you installed - everything else here is the standard Camera API.
#include "OV5640/ov5640.h"

#include "Config.h"

// What we pull out of one camera frame before turning it into a temperature.
struct FrameStats {
  float darkLevel;    // mean of the dark ROI, in counts
  float brightness;   // dark-subtracted signal-ROI mean, in counts (>= 0)
  uint8_t peak;       // brightest pixel in the signal ROI
  bool saturated;     // peak is hard against full scale
  bool valid;         // enough signal to treat as a real target
};

// ============================================================================
//  CameraSensor - OV5640 front-end
// ----------------------------------------------------------------------------
//  Captures grayscale frames over the STM32H7 DCMI peripheral and DMA (handled
//  inside the Camera library), then reduces each frame to brightness over a
//  signal ROI against a dark ROI. No pixel loop ever runs on the raw stream.
//
//  Note: the high-level Camera API exposes no exposure/gain lock, so the
//  OV5640's auto-exposure will fight an absolute brightness reading. For
//  quantitative temperature, disable AEC/AGC at the sensor (see
//  docs/calibration.md) and calibrate with it fixed.
// ============================================================================
class CameraSensor {
 public:
  // Brings the camera up at the configured resolution/format. False on failure.
  bool begin();

  // Grabs one DMA frame and fills `out` with ROI brightness stats. Returns
  // false on a capture timeout.
  bool capture(FrameStats& out);

 private:
  OV5640 sensor_;
  Camera cam_{sensor_};
  FrameBuffer fb_;  // the Camera library manages the backing DMA buffer

  // Mean (and optional peak) over an ROI of the grayscale frame.
  static float meanOverRoi(const uint8_t* img, uint16_t x, uint16_t y,
                           uint16_t w, uint16_t h, uint8_t* peakOut);
};
