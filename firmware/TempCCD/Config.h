#pragma once

#include <Arduino.h>

// ============================================================================
//  TempCCD - central configuration
// ----------------------------------------------------------------------------
//  Board : Arduino GIGA R1 WiFi (STM32H747XI, Cortex-M7 core)
//  Sensor: Toshiba TCD1304 linear CCD (3648 active photosites)
//  Output: 16x2 character LCD over I2C (PCF8574 backpack)
//
//  Everything that you might reasonably want to tweak lives here so the rest
//  of the code can stay generic. Anything marked CALIBRATE depends on your
//  optics and wiring and must be tuned against a known reference - see
//  docs/calibration.md.
// ============================================================================

// ---- CCD clock & readout timing --------------------------------------------
// The TCD1304 shifts one pixel out every four master-clock (fM) cycles, so the
// analog output (OS) settles at fM / 4. Keep fM inside the datasheet's
// 0.8-4 MHz window; 2 MHz gives a comfortable 500 kHz pixel rate.
static constexpr uint32_t CCD_MASTER_CLOCK_HZ = 2000000UL;
static constexpr uint32_t CCD_PIXEL_RATE_HZ   = CCD_MASTER_CLOCK_HZ / 4;  // 500 kHz

// Total clocked elements = 32 dummy + 3648 photosites + 14 trailing dummy.
static constexpr uint16_t CCD_TOTAL_PIXELS  = 3694;
static constexpr uint16_t CCD_ACTIVE_PIXELS = 3648;

// Light-shielded elements used as the dark (zero-light) reference. The leading
// dummies sit before the imaging area; 16..31 is a safe fully-shielded window
// with enough margin to tolerate a small fixed capture-phase offset.
static constexpr uint16_t CCD_DARK_FIRST = 16;
static constexpr uint16_t CCD_DARK_LAST  = 31;

// First/last photosite treated as illuminated signal. Trim the optical black
// and the far edge where vignetting bites. CALIBRATE with the serial dump.
static constexpr uint16_t CCD_SIGNAL_FIRST = 40;
static constexpr uint16_t CCD_SIGNAL_LAST  = 3660;

// One readout streams every pixel out at the pixel rate, so the frame period
// is fixed by the pixel count (~7.39 ms). Exposure equals one frame here;
// finer exposure control would drive the SH electronic shutter inside a frame
// (see docs/calibration.md).
static constexpr uint32_t CCD_FRAME_PERIOD_US =
    static_cast<uint32_t>(static_cast<uint64_t>(CCD_TOTAL_PIXELS) * 1000000ULL /
                          CCD_PIXEL_RATE_HZ);

// ---- Pin map (Arduino GIGA R1) ---------------------------------------------
// fM needs a hardware-PWM capable pin; SH/ICG are plain GPIO. If fM does not
// oscillate, the pin's timer probably clashes with the ADC/Ticker - move it to
// another PWM pin (see docs/wiring.md).
static constexpr pin_size_t PIN_CCD_FM  = D2;   // master clock -> CCD fM / MCLK pad
static constexpr pin_size_t PIN_CCD_SH  = D3;   // shift gate    -> CCD SH pad
static constexpr pin_size_t PIN_CCD_ICG = D4;   // integ. clear  -> CCD ICG pad
static constexpr pin_size_t PIN_CCD_OS  = A0;   // analog output <- CCD OS pad (buffered)

// ---- ADC / DMA capture -----------------------------------------------------
// AdvancedADC drives the STM32 ADC from a hardware timer and lands every sample
// in a DMA ring buffer, so the CPU never touches the sample stream. One full
// buffer == one CCD frame.
static constexpr uint8_t ADC_RESOLUTION_BITS    = 16;  // H7 ADC is natively 16-bit
static constexpr size_t  ADC_SAMPLES_PER_BUFFER = CCD_TOTAL_PIXELS;
static constexpr size_t  ADC_BUFFER_COUNT       = 4;   // DMA ring depth

// The OS output is inverted: brighter light pulls the voltage DOWN. We undo
// that in software so "brightness" climbs with light.
static constexpr bool     CCD_OUTPUT_INVERTED = true;
static constexpr uint16_t ADC_FULL_SCALE      = (1u << ADC_RESOLUTION_BITS) - 1;

// ---- Pyrometry (brightness -> temperature) ---------------------------------
// Wien approximation of Planck's law over the CCD's narrow effective band:
//     S = G * exp(-c2 / (lambda * T))   =>   1/T = (A - ln S) / B
// with B = c2 / lambda_eff and A = ln G. The defaults below are placeholders
// for a ~600 nm effective wavelength and an arbitrary gain - they WILL be wrong
// for your rig. CALIBRATE with two known temperatures (docs/calibration.md).
static constexpr float PYRO_C2_NM_K       = 1.4388e7f;        // c2 in nm*K
static constexpr float PYRO_LAMBDA_EFF_NM = 600.0f;           // CALIBRATE
static constexpr float PYRO_DEFAULT_B     = PYRO_C2_NM_K / PYRO_LAMBDA_EFF_NM;  // ~23980 K
static constexpr float PYRO_DEFAULT_A     = 22.9f;            // CALIBRATE (ln gain)

// Plausibility gate on the result, plus the minimum dark-subtracted signal we
// trust as a real target rather than noise.
static constexpr float PYRO_MIN_KELVIN     = 500.0f;   // below this a Si CCD sees nothing
static constexpr float PYRO_MAX_KELVIN     = 4000.0f;
static constexpr float PYRO_MIN_BRIGHTNESS = 8.0f;     // counts above dark

// ---- LCD -------------------------------------------------------------------
static constexpr uint8_t LCD_I2C_ADDRESS = 0x27;  // common PCF8574 default (0x3F on some)
static constexpr uint8_t LCD_COLUMNS     = 16;
static constexpr uint8_t LCD_ROWS        = 2;

// ---- Application -----------------------------------------------------------
static constexpr uint32_t DISPLAY_REFRESH_MS = 250;     // 4 Hz, easy on the eye
static constexpr uint32_t SERIAL_BAUD        = 115200;

// Set to 1 to stream raw pixel values over Serial for tuning the dark/signal
// windows. Leave at 0 for normal operation.
#define DEBUG_DUMP_FRAME 0
