#pragma once

#include <Arduino.h>

#include "Config.h"

// ============================================================================
//  Pyrometer - turns measured brightness into temperature
// ----------------------------------------------------------------------------
//  Uses the Wien approximation of Planck's law over the CCD's narrow effective
//  band:
//        S = G * exp(-c2 / (lambda * T))    =>    1/T = (A - ln S) / B
//  where B = c2 / lambda_eff and A = ln G folds in optics, emissivity and gain.
//  A and B come from a two-point calibration against known temperatures.
//
//  Note: a silicon sensor only sees an object once it glows (roughly >500 C),
//  so this measures hot/incandescent targets, not room-temperature surfaces.
// ============================================================================
class Pyrometer {
 public:
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
