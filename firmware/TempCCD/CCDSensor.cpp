#include "CCDSensor.h"

#include <chrono>

CCDSensor* CCDSensor::instance_ = nullptr;

namespace {

// Map a bit depth to the AdvancedAnalog resolution enum.
uint32_t adcResolutionEnum(uint8_t bits) {
  switch (bits) {
    case 8:  return AN_RESOLUTION_8;
    case 10: return AN_RESOLUTION_10;
    case 12: return AN_RESOLUTION_12;
    case 14: return AN_RESOLUTION_14;
    case 16: return AN_RESOLUTION_16;
    default: return AN_RESOLUTION_12;
  }
}

}  // namespace

bool CCDSensor::begin() {
  instance_ = this;

  // fM: free-running 50% square wave. period() takes seconds.
  masterClock_ = new mbed::PwmOut(digitalPinToPinName(PIN_CCD_FM));
  masterClock_->period(1.0f / static_cast<float>(CCD_MASTER_CLOCK_HZ));
  masterClock_->write(0.5f);

  // Idle states: ICG high, SH low.
  clearGate_ = new mbed::DigitalOut(digitalPinToPinName(PIN_CCD_ICG), 1);
  shiftGate_ = new mbed::DigitalOut(digitalPinToPinName(PIN_CCD_SH), 0);

  // Start the DMA-backed ADC at the pixel rate. begin() returns 1 on success.
  if (!adc_.begin(adcResolutionEnum(ADC_RESOLUTION_BITS), CCD_PIXEL_RATE_HZ,
                  ADC_SAMPLES_PER_BUFFER, ADC_BUFFER_COUNT)) {
    return false;
  }

  // Fire the readout sequence once per frame, now that the clocks are live.
  readoutTimer_.attach(&CCDSensor::readoutTrampoline,
                       std::chrono::microseconds(CCD_FRAME_PERIOD_US));
  return true;
}

bool CCDSensor::frameReady() {
  return adc_.available();
}

bool CCDSensor::readFrame(uint16_t* dest) {
  if (!adc_.available()) {
    return false;
  }

  SampleBuffer buf = adc_.read();
  const size_t n = buf.size();
  for (size_t i = 0; i < n && i < CCD_TOTAL_PIXELS; ++i) {
    const uint16_t raw = static_cast<uint16_t>(buf[i]);
    dest[i] = CCD_OUTPUT_INVERTED ? static_cast<uint16_t>(ADC_FULL_SCALE - raw)
                                  : raw;
  }
  buf.release();  // return the DMA buffer to the pool
  return true;
}

void CCDSensor::readoutTrampoline() {
  if (instance_ != nullptr) {
    instance_->pulseReadout();
  }
}

// Datasheet readout sequence: drop ICG, pulse SH while ICG is low, then raise
// ICG to begin clocking pixels out. The waits are short busy-loops; the whole
// thing is a few microseconds once per ~7.4 ms frame.
void CCDSensor::pulseReadout() {
  *clearGate_ = 0;        // ICG low
  delayMicroseconds(1);   // setup
  *shiftGate_ = 1;        // SH high
  delayMicroseconds(2);   // SH width
  *shiftGate_ = 0;        // SH low
  delayMicroseconds(1);   // hold
  *clearGate_ = 1;        // ICG high -> readout begins
}
