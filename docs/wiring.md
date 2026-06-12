# Wiring

Two connections, and one of them is a ribbon you just plug in. The OV5640 is
digital, so there's no analog front-end, no clock to generate, and no pins to
level-shift.

## Camera to GIGA

The OV5640 module seats into the GIGA R1's dedicated camera connector. That one
connector carries the whole DCMI bus — pixel data, pixel clock, HSYNC, VSYNC,
the SCCB control lines, the master clock and power — so there is nothing to wire
by hand.

| Step | Notes |
|---|---|
| Power off the GIGA first | never seat or unseat the ribbon live |
| Check orientation | the FPC contacts face the way the connector expects; don't force it |
| Latch the connector | flip the retainer back down so the ribbon can't creep out |
| Support the module | don't let it dangle on the ribbon — strain cracks the FPC |

Use an OV5640 module meant for the GIGA camera connector (e.g. Arducam's). The
camera-connector pinout is fixed in hardware, so there's nothing to set in
`Config.h` for it beyond the capture mode.

## LCD to GIGA

| LCD backpack | GIGA pin |
|--------------|----------|
| VCC          | 5V       |
| GND          | GND      |
| SDA          | SDA      |
| SCL          | SCL      |

The backpack's PCF8574 is usually at `0x27`, sometimes `0x3F`. If the screen
stays blank, scan the bus and set `LCD_I2C_ADDRESS` in `Config.h` to match.

## Power and grounding

- The camera is powered through its connector, so it needs nothing extra.
- Give the LCD a clean 5 V and share a common ground with the GIGA.
- Keep the camera ribbon and the lens away from the hot zone — see
  [mounting.md](mounting.md). The ribbon's insulation and the module itself are
  the temperature-sensitive parts, not some op-amp.

## Lens and focus

The OV5640 ships with a small lens already. For pyrometry you want the hot target
focused onto the middle of the frame so it fills the signal ROI. Rotate the lens
to focus, and frame the target with the help of the serial brightness readout
(see [calibration.md](calibration.md)). A longer lens lets you stand further back
from the heat for the same framing, which is usually what you want.
