#include "Pyrometer.h"

#include <math.h>

FrameStats Pyrometer::analyze(const uint16_t* frame) const {
  // Dark reference: mean of the light-shielded window.
  uint32_t darkSum = 0;
  for (uint16_t i = CCD_DARK_FIRST; i <= CCD_DARK_LAST; ++i) {
    darkSum += frame[i];
  }
  const uint16_t darkCount = CCD_DARK_LAST - CCD_DARK_FIRST + 1;
  const float dark = static_cast<float>(darkSum) / darkCount;

  // Signal: mean and peak over the illuminated window.
  uint32_t sigSum = 0;
  uint16_t peak = 0;
  for (uint16_t i = CCD_SIGNAL_FIRST; i <= CCD_SIGNAL_LAST; ++i) {
    const uint16_t v = frame[i];
    sigSum += v;
    if (v > peak) {
      peak = v;
    }
  }
  const uint16_t sigCount = CCD_SIGNAL_LAST - CCD_SIGNAL_FIRST + 1;
  const float sigMean = static_cast<float>(sigSum) / sigCount;

  FrameStats s;
  s.darkLevel = dark;
  s.brightness = sigMean - dark;
  if (s.brightness < 0.0f) {
    s.brightness = 0.0f;
  }
  s.peak = peak;
  s.saturated = peak >= static_cast<uint16_t>(ADC_FULL_SCALE * 0.98f);
  s.valid = (s.brightness >= PYRO_MIN_BRIGHTNESS) && !s.saturated;
  return s;
}

float Pyrometer::temperatureK(float brightness) const {
  if (brightness <= 0.0f) {
    return NAN;
  }
  // 1/T = (A - ln S) / B. A larger S (more light) shrinks the denominator and
  // raises T, as expected. A denominator <= 0 means brightness has run past the
  // model's gain - treat it as out of range rather than returning nonsense.
  const float denom = a_ - logf(brightness);
  if (denom <= 0.0f) {
    return NAN;
  }
  const float kelvin = b_ / denom;
  if (kelvin < PYRO_MIN_KELVIN || kelvin > PYRO_MAX_KELVIN) {
    return NAN;
  }
  return kelvin;
}

void Pyrometer::setEffectiveWavelength(float lambda_nm) {
  if (lambda_nm > 0.0f) {
    b_ = PYRO_C2_NM_K / lambda_nm;
  }
}

void Pyrometer::setCalibration(float a, float b) {
  a_ = a;
  b_ = b;
}

bool Pyrometer::calibrateTwoPoint(float brightness1, float kelvin1,
                                  float brightness2, float kelvin2) {
  if (brightness1 <= 0.0f || brightness2 <= 0.0f || kelvin1 <= 0.0f ||
      kelvin2 <= 0.0f) {
    return false;
  }

  // 1/T1 - 1/T2 = (ln S2 - ln S1) / B  =>  B = (ln S2 - ln S1) / (1/T1 - 1/T2)
  const float invDiff = (1.0f / kelvin1) - (1.0f / kelvin2);
  if (fabsf(invDiff) < 1e-9f) {
    return false;
  }
  const float b = (logf(brightness2) - logf(brightness1)) / invDiff;
  if (!isfinite(b) || b <= 0.0f) {
    return false;
  }

  a_ = logf(brightness1) + b / kelvin1;
  b_ = b;
  return true;
}
