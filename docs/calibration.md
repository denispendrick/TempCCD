# Calibration

The firmware ships with placeholder constants, so the temperature it prints
before you calibrate is meaningless. With the OV5640 there's an extra job before
any of it matters: stop the camera from auto-adjusting. Three steps — lock
exposure, frame the regions of interest, then fit the curve.

## 1. Lock exposure and gain (do this first)

This is the big one. The OV5640 runs auto-exposure (AEC) and auto-gain (AGC) by
default, so it quietly re-brightens the scene frame to frame. While that's on,
your brightness reading tracks the camera's mood, not the target's temperature,
and no calibration will hold.

The high-level Camera API can't turn it off, so you write the sensor's registers
directly. Most OV5640 drivers expose a register-write call; the registers are:

| Register | Purpose | Set to |
|---|---|---|
| `0x3503` | manual AEC + AGC enable | `0x03` (both manual) |
| `0x3500`–`0x3502` | exposure (20-bit, 1/16-line units) | a fixed value |
| `0x350A`–`0x350B` | gain (10-bit) | a fixed value |

Pick an exposure/gain that puts your target mid-scale (peak well under 250, see
below) and freeze them there. From then on the camera is a stable light meter.
If you change exposure or gain later, you recalibrate.

## 2. Frame the regions of interest

Open the serial monitor at 115200. Each reading prints `dark`, `bright` and
`peak`. Aim with those numbers:

- **Signal ROI** (`CAM_ROI_*`): move/focus the camera so the glow sits inside
  the box. `bright` should climb and `peak` should rise well clear of `dark`
  without pinning at 250.
- **Dark ROI** (`CAM_DARK_*`): put it on a corner that stays genuinely dark.
  `dark` should sit low and steady.

The defaults are a centred 64x64 signal box and a 32x32 dark corner on a 320x240
frame. Nudge the coordinates in `Config.h` until the numbers behave, and keep
the boxes inside the frame.

## 3. Fit brightness to temperature

The model is the Wien approximation of Planck's law:

```
1/T = (A - ln S) / B
```

`S` is the dark-subtracted brightness the sketch prints as `bright=`, `T` is in
kelvin. You find `A` and `B` from two measurements at known temperatures.

1. Heat your target to a known temperature `T1` (use a reference pyrometer or
   thermocouple) and note the `bright=` value — that's `S1`.
2. Do it again at a second, well-separated temperature `T2` to get `S2`.
3. Feed both pairs to the firmware once, in `setup()`:

```cpp
// temperatures in kelvin
pyrometer.calibrateTwoPoint(S1, T1_kelvin, S2, T2_kelvin);
```

`calibrateTwoPoint` solves `A` and `B` and returns `false` if the two points are
too close to be useful. Print `pyrometer.a()` and `pyrometer.b()` afterwards and
paste those numbers into `PYRO_DEFAULT_A` / `PYRO_DEFAULT_B` in `Config.h` so the
calibration sticks without redoing it every boot.

Two points are the minimum. If you can, take several across your range and fit
`1/T` against `ln S` as a straight line — the slope is `-1/B`, the intercept is
`A/B`, and more points average out noise.

### Effective wavelength (optional)

`B` is physically `c2 / lambda_eff`. Put a narrow-band filter in front of the
camera and you can pin `B` from the filter instead of fitting it:

```cpp
pyrometer.setEffectiveWavelength(650.0f);  // nm
```

Then only `A` is left to fit from a single known point.

## Things that will bite you

- **Auto-exposure creeping back.** If readings drift even though nothing moved,
  AEC/AGC is still on. Re-check step 1.
- **Saturation.** If `peak=` sits near 250 the reading is capped and the sketch
  shows `SATURATED`. Lower the exposure, stop down the optics, or add a neutral
  filter.
- **Emissivity.** The fit assumes your target radiates like the calibration
  source. A shiny surface reads low. Calibrate against something with similar
  emissivity, or against the real object.
- **It only sees glowing things.** A silicon sensor is blind to
  room-temperature thermal radiation. Below roughly 500 °C there is simply
  nothing in its band to measure.
