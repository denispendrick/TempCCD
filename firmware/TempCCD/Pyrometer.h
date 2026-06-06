#pragma once

#include <Arduino.h>

#include "Config.h"

// What we pull out of a single CCD frame before turning it into a temperature.
struct FrameStats {
  float darkLevel;    // mean of the shielded pixels, in counts
  float brightness;   // dark-subtracted signal, in counts (never negative)
  uint16_t peak;      // brightest raw count in the signal window
  bool saturated;     // peak is hard against full scale
  bool valid;         // enough signal to treat as a real target
};

// ============================================================================
//  Pyrometer - turns CCD brightness into temperature
// ----------------------------------------------------------------------------
//  Uses the Wien approximation of Planck's law over the CCD's narrow effective
//  band:
//        S = G * exp(-c2 / (lambda * T))    =>    1/T = (A - ln S) / B
//  where B = c2 / lambda_eff and A = ln G folds in optics, emissivity and gain.
//  A and B come from a two-point calibration against known temperatures.
//
//  Note: a silicon CCD only sees an object once it glows (roughly >500 C), so
//  this measures hot/incandescent targets, not room-temperature surfaces.
// ============================================================================
class Pyrometer {
 public:
  // Reduce a frame to dark level, brightness, peak and validity flags.
  FrameStats analyze(const uint16_t* frame) const;

  // Temperature in kelvin for a given brightness; NAN if outside the model's
  // valid domain or the plausibility window in Config.h.
  float temperatureK(float brightness) const;

  // Set the effective wavelength (nm); recomputes B = c2 / lambda.
  void setEffectiveWavelength(float lambda_nm);

  // Set the model constants directly.
  void setCalibration(float a, float b);

  // Solve A and B from two brightness/temperature pairs (kelvin). Returns false
  // for degenerate input (non-positive values or coincident points).
  bool calibrateTwoPoint(float brightness1, float kelvin1, float brightness2,
                         float kelvin2);

  float a() const { return a_; }
  float b() const { return b_; }

 private:
  float a_ = PYRO_DEFAULT_A;
  float b_ = PYRO_DEFAULT_B;
};
