# TempCCD

A non-contact thermometer built from a linear CCD and an Arduino GIGA R1. The
CCD looks at a hot, glowing target, the GIGA reads the sensor over DMA, and the
temperature lands on a small I2C LCD.

It works as an optical pyrometer. A silicon CCD only starts seeing an object
once it glows — very roughly above 500 °C — so this is for things like a kiln, a
heating element or molten metal, not for room-temperature surfaces.

The full write-up — principle, circuit, algorithm and program — is in
[docs/design.md](docs/design.md).

## How it works

```
TCD1304 CCD  ->  ADC + DMA  ->  brightness  ->  pyrometry  ->  16x2 LCD
```

1. The GIGA generates the three clocks the CCD needs (master clock, shift gate,
   integration clear gate) and shifts a full line of pixels out.
2. Every pixel is sampled by the STM32 ADC and dropped straight into a DMA ring
   buffer, so the CPU never sits in a read loop. One buffer is one frame.
3. The shielded pixels give a dark reference; the lit pixels give brightness.
4. Brightness is converted to temperature with a Wien-law model that you
   calibrate against two known temperatures.
5. The reading goes to the LCD and to the serial monitor.

The DMA capture uses Arduino's `AdvancedADC` library, which is the supported way
to do timer-driven, DMA-backed sampling on the GIGA and Portenta.

## Hardware

- Arduino GIGA R1 (STM32H747, Cortex-M7)
- Toshiba TCD1304 linear CCD (3648 active pixels)
- 16x2 character LCD with a PCF8574 I2C backpack
- A small analog front-end for the CCD output — see `docs/wiring.md`

**Heads up:** the GIGA's analog inputs are 3.3 V and *not* 5 V tolerant. The
TCD1304 output has to be buffered and shifted into 0–3.3 V before it touches
`A0`, or you will damage the board. The wiring guide covers this.

Pin assignments live at the top of `firmware/TempCCD/Config.h`.

## Build and flash

Install the core and libraries once:

```bash
arduino-cli core install arduino:mbed_giga
arduino-cli lib install Arduino_AdvancedAnalog
arduino-cli lib install "LiquidCrystal I2C"
```

Then compile and upload (swap in your port):

```bash
arduino-cli compile --fqbn arduino:mbed_giga:giga firmware/TempCCD
arduino-cli upload  --fqbn arduino:mbed_giga:giga -p /dev/ttyACM0 firmware/TempCCD
```

You can also just open `firmware/TempCCD/TempCCD.ino` in the Arduino IDE, pick
the GIGA R1 board, and add the two libraries from the Library Manager.

## Calibrating

Out of the box the temperature numbers are placeholders. Pyrometry depends on
your optics, the CCD window, and emissivity, so you need to calibrate against
two known temperatures before the readings mean anything. `docs/calibration.md`
walks through it, including how to use the built-in serial pixel dump to set the
dark and signal windows.

## Layout

```
firmware/TempCCD/   the sketch and its modules
  Config.h            pins, timing, ADC and pyrometry constants
  CCDSensor.*         CCD clocks + DMA ADC capture
  Pyrometer.*         brightness -> temperature
  TemperatureDisplay.*  the LCD
  TempCCD.ino         setup/loop glue
docs/               wiring and calibration guides
```
