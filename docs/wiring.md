# Wiring

Signals are referenced by name. Most TCD1304 breakout boards label their pads
`MCLK`/`fM`, `SH`, `ICG`, `OS`, `5V` and `GND` — match those rather than chasing
raw chip pin numbers, since they differ between boards.

## CCD to GIGA

| CCD pad      | GIGA pin | Notes                                            |
|--------------|----------|--------------------------------------------------|
| fM / MCLK    | D2       | 2 MHz master clock (hardware PWM)                |
| SH           | D3       | shift gate                                       |
| ICG          | D4       | integration clear gate                           |
| OS           | A0       | analog output — **through the front-end below**  |
| 5V           | 5V       | sensor supply                                    |
| GND          | GND      | common ground                                    |

The pins are set in `firmware/TempCCD/Config.h`. If `fM` never oscillates, its
timer is probably claimed by the ADC or the Ticker — move `PIN_CCD_FM` to a
different PWM pin and re-flash.

## Analog front-end (do not skip)

The GIGA's analog inputs run at **3.3 V and are not 5 V tolerant**. The TCD1304
`OS` output swings around a DC level near the sensor supply, which is well above
3.3 V, so it has to be conditioned before it reaches `A0`:

1. **Buffer** `OS` with a rail-to-rail op-amp (e.g. MCP6002) running off 3.3 V.
2. **Shift and scale** the signal so the full pixel swing maps into roughly
   0.1–3.2 V. A simple inverting/level-shift stage around the op-amp does this.
3. **Clamp** to 3.3 V (a small Schottky to the 3.3 V rail) as insurance.
4. Keep the op-amp output **low impedance** — the ADC samples fast, so a stiff
   driver gives cleaner pixels.

Remember the output is inverted (more light = lower `OS` voltage). The firmware
already flips this in software (`CCD_OUTPUT_INVERTED`), so wire for signal
integrity and let the code handle the sense.

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

- Give the CCD a clean 5 V; add a 100 nF decoupling cap close to its supply pin
  and a larger bulk cap (10 µF) nearby.
- Keep the analog ground return short and away from the digital clock traces —
  `fM` is a fast square wave and will couple into `OS` if they run together.
- One common ground between the GIGA, the CCD board and the LCD.
