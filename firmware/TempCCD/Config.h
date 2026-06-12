#pragma once

#include <Arduino.h>
#include "camera.h"  // CAMERA_* resolution and pixel-format constants

// ============================================================================
//  TempCCD - central configuration
// ----------------------------------------------------------------------------
//  Board : Arduino GIGA R1 WiFi (STM32H747XI, Cortex-M7 core)
//  Sensor: OmniVision OV5640 camera on the GIGA camera connector
//  Output: 16x2 character LCD over I2C (PCF8574 backpack)
//
//  Everything tunable lives here. Anything marked CALIBRATE depends on your
//  optics and framing and must be tuned against a known reference - see
//  docs/calibration.md.
// ============================================================================

// ---- Camera (OV5640 over the GIGA camera connector) ------------------------
// The Arduino Camera library captures frames over the STM32H7 DCMI peripheral
// straight into a DMA buffer, so the pixel stream never touches the CPU. We use
// 8-bit grayscale because all we need from each frame is brightness.
//
// Heads up: a low resolution keeps the frame small and the maths cheap; the hot
// target only has to fill the signal ROI below, not the whole frame.
static constexpr int32_t  CAM_RESOLUTION       = CAMERA_R320x240;
static constexpr uint16_t CAM_WIDTH            = 320;
static constexpr uint16_t CAM_HEIGHT           = 240;
static constexpr int32_t  CAM_PIXFORMAT        = CAMERA_GRAYSCALE;  // 8-bit luma
static constexpr int32_t  CAM_FPS              = 30;
static constexpr uint32_t CAM_GRAB_TIMEOUT_MS  = 1000;
static constexpr uint8_t  CAM_PIXEL_MAX        = 255;
static constexpr uint8_t  CAM_SATURATION_LEVEL = 250;  // peak at/above this = clipped

// Signal ROI - a box framed on the hot target, centred by default. CALIBRATE so
// it sits on the glow and nothing else.
static constexpr uint16_t CAM_ROI_X = 128;
static constexpr uint16_t CAM_ROI_Y = 88;
static constexpr uint16_t CAM_ROI_W = 64;
static constexpr uint16_t CAM_ROI_H = 64;

// Dark ROI - a corner kept out of the glow, used as the zero-light reference in
// the same frame (the CMOS equivalent of a CCD's shielded pixels).
static constexpr uint16_t CAM_DARK_X = 8;
static constexpr uint16_t CAM_DARK_Y = 8;
static constexpr uint16_t CAM_DARK_W = 32;
static constexpr uint16_t CAM_DARK_H = 32;

// ---- Pyrometry (brightness -> temperature) ---------------------------------
// Wien approximation of Planck's law over the camera's effective band:
//     S = G * exp(-c2 / (lambda * T))   =>   1/T = (A - ln S) / B
// with B = c2 / lambda_eff and A = ln G. The defaults are placeholders for a
// ~600 nm effective wavelength on the 8-bit brightness scale - they WILL be
// wrong for your rig. CALIBRATE with two known temperatures (docs/calibration).
static constexpr float PYRO_C2_NM_K       = 1.4388e7f;        // c2 in nm*K
static constexpr float PYRO_LAMBDA_EFF_NM = 600.0f;           // CALIBRATE
static constexpr float PYRO_DEFAULT_B     = PYRO_C2_NM_K / PYRO_LAMBDA_EFF_NM;  // ~23980 K
static constexpr float PYRO_DEFAULT_A     = 20.8f;            // CALIBRATE (ln gain, 8-bit scale)

// Plausibility gate on the result, plus the minimum dark-subtracted signal we
// trust as a real target rather than sensor noise.
static constexpr float PYRO_MIN_KELVIN     = 500.0f;   // below this a silicon sensor sees nothing
static constexpr float PYRO_MAX_KELVIN     = 4000.0f;
static constexpr float PYRO_MIN_BRIGHTNESS = 2.0f;     // counts above dark (8-bit)

// ---- LCD -------------------------------------------------------------------
static constexpr uint8_t LCD_I2C_ADDRESS = 0x27;  // common PCF8574 default (0x3F on some)
static constexpr uint8_t LCD_COLUMNS     = 16;
static constexpr uint8_t LCD_ROWS        = 2;

// ---- Application -----------------------------------------------------------
static constexpr uint32_t DISPLAY_REFRESH_MS = 250;     // 4 Hz, easy on the eye
static constexpr uint32_t SERIAL_BAUD        = 115200;
