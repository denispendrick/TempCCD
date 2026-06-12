# TempCCD

A non-contact thermometer built from an OV5640 camera and an Arduino GIGA R1.
The camera looks at a hot, glowing target, the GIGA reads the sensor over DMA,
and the temperature lands on a small I2C LCD.

It works as an optical pyrometer. A silicon sensor only starts seeing an object
once it glows — very roughly above 500 °C — so this is for things like a kiln, a
heating element or molten metal, not for room-temperature surfaces.

The full write-up — principle, circuit, algorithm and program — is in
[docs/design.md](docs/design.md).

## How it works

```
OV5640 camera  ->  DCMI + DMA frame  ->  ROI brightness  ->  pyrometry  ->  16x2 LCD
```

1. The OV5640 streams grayscale frames into the GIGA over the STM32H7's DCMI
   camera interface. The Arduino Camera library lands each frame in a DMA
   buffer, so the CPU never touches the pixel stream.
2. Brightness is read from a region of interest framed on the hot target, with a
   dark corner of the same frame as the zero-light reference.
3. Brightness is converted to temperature with a Wien-law model that you
   calibrate against two known temperatures.
4. The reading goes to the LCD and to the serial monitor.

**One important caveat.** The OV5640 runs its own auto-exposure and auto-gain,
and the high-level camera API gives you no way to lock them. Until you disable
AEC/AGC at the sensor, the brightness — and therefore the temperature — is only
relative, because the camera keeps re-brightening the scene on its own. See
[docs/calibration.md](docs/calibration.md) for how to pin exposure. It's the
price of using a consumer CMOS camera instead of a raw sensor.

## Hardware

- Arduino GIGA R1 (STM32H747, Cortex-M7)
- OmniVision OV5640 camera module for the GIGA camera connector
- 16x2 character LCD with a PCF8574 I2C backpack

No analog front-end this time — the camera is digital and seats straight into
the GIGA's camera connector, so there's nothing to buffer or level-shift.

Camera mode and the regions of interest live at the top of
`firmware/TempCCD/Config.h`.

Measuring something genuinely hot? `docs/mounting.md` covers how to mount the
camera and shield the sensor so it survives the heat and stays clean.

## Build and flash

The generic Camera API ships with the GIGA core, but the **OV5640 driver does
not** — the stock library only carries Himax, GC2145 and OV7670. Install the
camera library that adds the OV5640 for the GIGA (Arducam's), and make the
include and class at the top of `firmware/TempCCD/CameraSensor.h` match it.

```bash
arduino-cli core install arduino:mbed_giga
arduino-cli lib install "LiquidCrystal I2C"
# plus your OV5640-for-GIGA camera library (e.g. Arducam's)
```

Then compile and upload (swap in your port):

```bash
arduino-cli compile --fqbn arduino:mbed_giga:giga firmware/TempCCD
arduino-cli upload  --fqbn arduino:mbed_giga:giga -p /dev/ttyACM0 firmware/TempCCD
```

You can also open `firmware/TempCCD/TempCCD.ino` in the Arduino IDE, pick the
GIGA R1 board, and add the libraries from the Library Manager.

## Calibrating

Out of the box the temperature numbers are placeholders, and with auto-exposure
still on they'll wander. The order that actually works: fix the OV5640's
exposure and gain, frame the regions of interest, then calibrate against two
known temperatures. [docs/calibration.md](docs/calibration.md) walks through all
three.

## Layout

```
firmware/TempCCD/   the sketch and its modules
  Config.h            camera mode, regions of interest, pyrometry constants
  CameraSensor.*      OV5640 capture (DCMI + DMA) and ROI brightness
  Pyrometer.*         brightness -> temperature
  TemperatureDisplay.*  the LCD
  TempCCD.ino         setup/loop glue
docs/               design, wiring, calibration and mounting guides
```
