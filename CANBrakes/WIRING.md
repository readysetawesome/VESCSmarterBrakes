# CANBrakes Wiring

## Components

| Part | Details |
|------|---------|
| CAN module | Longan Labs Serial CAN Bus Module v1.3 (Aug 2020) — SKU 1030001 |
| MCU | ATmega168PA @ 16 MHz (YXC 16OSDF external crystal) |
| CAN controller | MCP2515, CS = pin 9 |
| CAN transceiver | MCP2551 |
| LED driver | LuxDrive MiniPuck F004-D-P-350, 350 mA constant current, high-high PWM mode |
| Dimmer signal diode | 1N4148 (signal diode, anode toward Arduino, cathode toward MiniPuck) |
| CAN termination | P1 solder pad bridged (adds onboard 120 Ω termination resistor) |

## Pin Assignments

| Pin | Grove label | Function |
|-----|-------------|----------|
| 0 | RX | SoftPWM dimmer output → MiniPuck PWM input |
| 1 | TX | Mode button input (INPUT_PULLUP, active LOW) |
| 3 | — | Onboard status LED (active HIGH) |
| 9 | — | MCP2515 SPI chip select (internal, not broken out) |

## Wiring Diagram

```
  12V supply ──────────────────────────────────── MiniPuck Vin (+)
  GND ─────────────────────────────────────────── MiniPuck GND (−)
                                                   MiniPuck OUT+ ── LED strip +
                                                   MiniPuck OUT− ── LED strip −

  Arduino pin 0 ──[ 1N4148 ]──────────────────── MiniPuck PWM input
                    anode→   cathode→
  Arduino GND ─────────────────────────────────── MiniPuck GND
  12V GND ─────────────────────────────────────── MiniPuck GND (all grounds must share a common node)

  Mode button ── Arduino pin 1 (one side)
  GND ─────────── Arduino pin 1 (other side)   ← internal pull-up, press = LOW

  VESC CAN H ── CAN module CAN H
  VESC CAN L ── CAN module CAN L
```

## Key Notes

- **MiniPuck high-high mode**: PWM input HIGH = full brightness, LOW = off. The 1N4148
  drops ~0.7 V, so the Arduino's 5 V HIGH arrives as ~4.3 V — still a solid HIGH.
  The diode prevents the MiniPuck's 12 V-referenced circuitry from backfeeding into
  the Arduino's 5 V GPIO.

- **SoftPWM inversion**: `VESCSmarterBrakes` calls `SoftPWMSet(pin, 255 - value)` because
  the internal brightness constants use 0 = full on, 255 = off. The inversion maps that
  back to SoftPWM's 0 = off, 255 = full on convention.

- **CAN termination**: The bus needs 120 Ω at each end. Bridge the P1 solder pad on the
  CAN module to enable the onboard resistor. The VESC has its own termination built in.

- **CAN baud rate**: 500 kbps (VESC default). Set in `CANBrakes.ino` as `CAN_500KBPS`.

- **Multi-VESC**: `vesc_can.h` accepts Status frames from any controller ID on the bus.
  Brakes activate if any motor reports negative current (regen braking).

- **Programming**: FTDI adapter via Grove connector (TX→RX, RX→TX, GND, 5 V).
  No DTR on Grove — manually press RESET on the board when "Uploading..." appears.

## Sources

- [Longan Labs Serial CAN Bus Module v1.3 schematic & firmware](https://github.com/Longan-Labs/Serial_CAN_Bus_Module)
- [LuxDrive MiniPuck F004-D-P-350 datasheet](https://www.ledsupply.com/content/pdf/luxdrive-minipuck-datasheet.pdf)
- [SoftPWM library (bhagman)](https://github.com/bhagman/SoftPWM)
- [mcp_can library (coryjfowler)](https://github.com/coryjfowler/MCP_CAN_lib)
- [VESC CAN protocol — bldc source](https://github.com/vedderb/bldc/blob/master/comm/comm_can.c)
