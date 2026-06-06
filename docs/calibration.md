# Calibration

The firmware ships with placeholder constants, so the temperature it prints
before you calibrate is meaningless. There are two short jobs: line up the pixel
windows, then fit the brightness-to-temperature curve.

## 1. Set the dark and signal windows

Turn on the pixel dump in `Config.h`:

```c
#define DEBUG_DUMP_FRAME 1
```

Re-flash, open the serial monitor at 115200, and you'll get a full line of pixel
values about once a second. Point the CCD at your target and look at the shape:

- The first few dozen pixels are dummy/shielded and should sit flat and low —
  that's your **dark** region.
- The lit pixels rise into a hump or plateau — that's your **signal** region.

Set `CCD_DARK_FIRST/LAST` over a flat shielded stretch and
`CCD_SIGNAL_FIRST/LAST` over the lit area, trimming the very edges. Then set
`DEBUG_DUMP_FRAME` back to `0` and re-flash.

## 2. Fit brightness to temperature

The model is the Wien approximation of Planck's law:

```
1/T = (A - ln S) / B
```

`S` is the dark-subtracted brightness the sketch prints as `bright=`, `T` is in
kelvin. You find `A` and `B` from two measurements at known temperatures.

1. Heat your target to a known temperature `T1` (use a reference pyrometer or
   thermocouple). Note the `bright=` value — that's `S1`.
2. Do it again at a second, well-separated temperature `T2` to get `S2`.
3. Feed both pairs to the firmware once, in `setup()`:

```cpp
// temperatures in kelvin
pyrometer.calibrateTwoPoint(S1, T1_kelvin, S2, T2_kelvin);
```

`calibrateTwoPoint` solves `A` and `B` and returns `false` if the two points are
too close to be useful. Print `pyrometer.a()` and `pyrometer.b()` after the
call, then paste those numbers into `PYRO_DEFAULT_A` / `PYRO_DEFAULT_B` in
`Config.h` so the calibration sticks without redoing it every boot.

Two points are the minimum. If you can, take several across your range and fit
`1/T` against `ln S` as a straight line — the slope is `-1/B` and the intercept
is `A/B`. More points average out noise.

## 3. Effective wavelength (optional)

`B` is physically `c2 / lambda_eff`. If you put a narrow-band filter in front of
the CCD you can pin `B` from the filter instead of fitting it:

```cpp
pyrometer.setEffectiveWavelength(650.0f);  // nm
```

Then only `A` is left to fit from a single known point.

## 4. Sanity limits

`PYRO_MIN_KELVIN` / `PYRO_MAX_KELVIN` gate out nonsense results, and
`PYRO_MIN_BRIGHTNESS` decides how much signal counts as a real target. Widen or
tighten these to match what you're actually measuring.

## Things that will bite you

- **Saturation.** If `peak=` is pinned near full scale the reading is capped and
  the sketch shows `SATURATED`. Stop down the optics or add a neutral filter.
  Exposure here is one frame (~7.4 ms); shortening it means driving the SH
  electronic shutter, which is left as an extension.
- **Emissivity.** The fit assumes your target radiates like the calibration
  source. A shiny surface reads low. Calibrate against something with similar
  emissivity, or against the real object.
- **It only sees glowing things.** A silicon CCD is blind to room-temperature
  thermal radiation. Below roughly 500 °C there is simply nothing in its band to
  measure.
