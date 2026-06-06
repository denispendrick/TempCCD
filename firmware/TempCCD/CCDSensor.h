#pragma once

#include <Arduino.h>
#include <mbed.h>
#include <Arduino_AdvancedAnalog.h>

#include "Config.h"

// ============================================================================
//  CCDSensor - TCD1304 linear CCD front-end
// ----------------------------------------------------------------------------
//  Generates the three CCD clocks and pulls the pixel stream off the analog
//  output through the STM32 ADC + DMA. The sample path is entirely hardware:
//
//    * fM  (master clock) - free-running hardware PWM at CCD_MASTER_CLOCK_HZ
//    * SH  (shift gate)   - pulsed once per frame to start a new readout
//    * ICG (integ. clear) - framed around the SH pulse
//    * OS  (output)       - sampled at fM/4 by AdvancedADC straight into DMA
//
//  The ADC streams continuously into a DMA ring while a Ticker fires the SH/ICG
//  sequence every frame period. Both advance at the same pixel rate, so each
//  DMA buffer lines up with one frame at a constant offset that the dark/signal
//  windows in Config.h are sized to absorb.
// ============================================================================
class CCDSensor {
 public:
  // Brings up the clocks and the DMA-backed ADC. Returns false if the ADC
  // refuses to start.
  bool begin();

  // Non-blocking: true when a fresh DMA frame is waiting to be read.
  bool frameReady();

  // Copies the newest frame into dest (length CCD_TOTAL_PIXELS) as
  // light-positive counts (the OS inversion is already undone) and hands the
  // DMA buffer back to the pool. Returns false if no frame was ready.
  bool readFrame(uint16_t* dest);

 private:
  AdvancedADC adc_{PIN_CCD_OS};
  mbed::PwmOut* masterClock_ = nullptr;
  mbed::DigitalOut* shiftGate_ = nullptr;
  mbed::DigitalOut* clearGate_ = nullptr;
  mbed::Ticker readoutTimer_;

  // The Ticker needs a plain function, so we trampoline through a singleton.
  static CCDSensor* instance_;
  static void readoutTrampoline();
  void pulseReadout();  // ICG/SH sequence, runs in interrupt context
};
