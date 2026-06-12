#include "CameraSensor.h"

bool CameraSensor::begin() {
  // DCMI + DMA capture is set up inside the library; we just pick the mode.
  return cam_.begin(CAM_RESOLUTION, CAM_PIXFORMAT, CAM_FPS);
}

bool CameraSensor::capture(FrameStats& out) {
  // grabFrame returns 0 on success; non-zero is a timeout/error.
  if (cam_.grabFrame(fb_, CAM_GRAB_TIMEOUT_MS) != 0) {
    return false;
  }
  const uint8_t* img = fb_.getBuffer();

  uint8_t darkPeak = 0;
  const float dark =
      meanOverRoi(img, CAM_DARK_X, CAM_DARK_Y, CAM_DARK_W, CAM_DARK_H, &darkPeak);

  uint8_t peak = 0;
  const float signal =
      meanOverRoi(img, CAM_ROI_X, CAM_ROI_Y, CAM_ROI_W, CAM_ROI_H, &peak);

  out.darkLevel = dark;
  out.brightness = signal - dark;
  if (out.brightness < 0.0f) {
    out.brightness = 0.0f;
  }
  out.peak = peak;
  out.saturated = peak >= CAM_SATURATION_LEVEL;
  out.valid = (out.brightness >= PYRO_MIN_BRIGHTNESS) && !out.saturated;
  return true;
}

// Grayscale frame, row-major, one byte per pixel, stride == CAM_WIDTH.
float CameraSensor::meanOverRoi(const uint8_t* img, uint16_t x, uint16_t y,
                                uint16_t w, uint16_t h, uint8_t* peakOut) {
  uint32_t sum = 0;
  uint8_t peak = 0;
  for (uint16_t row = y; row < y + h; ++row) {
    const uint8_t* p = img + static_cast<uint32_t>(row) * CAM_WIDTH + x;
    for (uint16_t col = 0; col < w; ++col) {
      const uint8_t v = p[col];
      sum += v;
      if (v > peak) {
        peak = v;
      }
    }
  }
  if (peakOut != nullptr) {
    *peakOut = peak;
  }
  return static_cast<float>(sum) / (static_cast<uint32_t>(w) * h);
}
