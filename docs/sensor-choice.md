# Choosing the sensor: TCD1304 vs OV5640

This project started on a Toshiba TCD1304 linear CCD and moved to an OmniVision
OV5640 camera. The two pull in opposite directions, so here's the reasoning in
one place. Both meet the DMA requirement on the GIGA — the real choice is signal
quality versus convenience.

## At a glance

| | TCD1304 (linear CCD) | OV5640 (CMOS camera) |
|---|---|---|
| Sensor | 1D line, 3648 pixels | 2D image, 5 MP |
| Output | analog | digital (DCMI) |
| DMA path on the GIGA | AdvancedADC | DCMI |
| Extra hardware | clock generation + a 3.3 V analog front-end | just the ribbon |
| Exposure | you set the integration time | automatic (must disable AEC/AGC) |
| As a radiometer | faithful and repeatable | rough; sits behind image processing |
| What you see | a line to sight across the target | an image to frame and track |

## TCD1304 (linear CCD)

It's a raw, honest light sensor. One line of 3648 pixels, analog out, and nothing
sitting between the photons and your number — no auto-exposure, no gain games, no
image processing. You set the integration time yourself, so the brightness
reading is faithful and repeatable. That's exactly what you want when you're
turning brightness into a temperature, and it's what makes a calibration actually
hold.

The cost is hardware. You generate the sensor's clocks (fM/SH/ICG), and its
analog output has to be buffered and level-shifted into the GIGA's 3.3 V ADC,
which isn't 5 V tolerant. More to build — and it's a line, not a picture, so you
sight that line across the hot spot.

## OV5640 (CMOS camera)

It's a proper 5 MP camera. Digital, plugs straight into the GIGA's camera
connector, and the DCMI peripheral DMAs whole frames for you — almost no
electronics to build. You get a full 2D image, so you can actually see the
target, frame a region of interest, and even track the hot spot as it moves.

The catch is that everything making it a good camera makes it a worse
thermometer: it auto-exposes, auto-gains and processes the image, so out of the
box the brightness chases the camera's own settings instead of the temperature.
You can rein that in by disabling AEC/AGC in the sensor registers (see
[calibration.md](calibration.md)), but even locked down it's consumer-grade — a
rougher radiometer than the CCD.

## Bottom line

- Want the most accurate, stable temperature and don't mind the extra
  electronics? The TCD1304 is the better radiometer.
- Want it wired up fast, with a live image to aim and track, and you're fine
  locking the exposure and accepting consumer-grade accuracy? The OV5640 is the
  easier, more flexible path.

Either way the DMA box is ticked — AdvancedADC for the CCD, DCMI for the camera.
This build runs on the OV5640 for the convenience and the live image; if you need
laboratory-grade numbers, the linear CCD is the one to reach for.
