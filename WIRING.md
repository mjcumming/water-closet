# A16 channels checked on the bench

October 6, 2026. Read from the live board. This is the installed wiring. CONNECTIONS.md uses the same terminals.

Left and right are the two end positions of each selector. The center position leaves both of that selector's inputs open. Facing the knob, left is On and right is Auto.

The raw input and output page is `kc868-a16.yaml`. `kc868-a16-simple.yaml` is the first hooked-up version: no automatic modes, just these connections.

## Inputs

| Input | Function |
|---|---|
| 1 | Hot water tank, left |
| 2 | Hot water tank, right |
| 3 | Water pump, left |
| 4 | Water pump, right |
| 5 | Spin filter flush button |
| 6 | UV button |
| 7–16 | Not assigned |

## Outputs

Four LED outputs, one for each illuminated control. On the simple page, the pump selector drives output 9, the tank selector drives output 10, the spin filter flush button toggles output 11, and the UV button toggles output 12.

| Output | Function |
|---|---|
| 9 | Pump selector LED |
| 10 | Hot water tank LED |
| 11 | Spin filter flush LED |
| 12 | UV button LED |

Three contactor outputs. The simple page leaves these as manual switches. It does not turn them on from the selectors.

| Output | Function |
|---|---|
| 1 | Pump contactor |
| 2 | Hot water tank contactor |
| 3 | UV contactor |

Outputs 4 and 5 are the spin-flush valve and the system-flush valve. Outputs 6–8 and 13–16 are not assigned.

Red and black on each selector are the LED pair, not the position contacts. All four LEDs are 12 V DC.

## What the simple config does

- Tank left or tank right turns on output 10, the tank LED. Both open turns it off.
- Pump left or pump right turns on output 9, the pump LED. Both open turns it off.
- The spin filter flush button toggles output 11, the spin filter flush LED.
- The UV button toggles output 12, the UV button LED.

Each press flips that output and leaves it there.

## Pressure sensor

`kc868-a16-pressure.yaml` is only a signal check for the LEFOO T2000. Flashing it takes the switch page offline until `kc868-a16-simple.yaml` is flashed again.

The sensor is two-wire, 4–20 mA, 0–100 PSI. A1 on the current A16 schematic is the 4–20 mA input: 0 Ω in series and 150 Ω to analog ground. The loop current has to pass through that 150 Ω. The 24 V supply never lands on A1.

The sensor leads are not a resistance check. Unpowered, the two loop wires of a 4–20 mA transmitter read open. October 6: the LEFOO leads read open on the meter, which is the expected unpowered result. A near-short between those leads would be the bad reading.

October 6: A1 to the 12 V supply negative measured 148.8 Ω. That is the onboard 150 Ω shunt, so A1 is the current input. Add no extra resistor. The YAML's 150 Ω conversion matches this board.

The owned unit is the Amazon LEFOO T2000, 0–100 PSI, 4–20 mA, 8–36 VDC, Hirschmann plug and 1 m cable. The cable in hand is two insulated wires plus a bare shield: red, black, and uncovered ground. The red / blue / green chart is a different LEFOO cable and does not apply.

| Wire | Connection |
|---|---|
| Red | Converter +24 V |
| Black | A16 analog A1 |
| Bare shield | Analog GND only. It is not one of the two loop wires. |

Converter negative goes to analog GND. Converter input is the existing +12 V and DC 0 V. If pin 2 goes to GND instead of A1, the current skips the shunt and the page stays near 0 V.

October 6: with red on +24 V and black on A1, the open-air reading was about 0.63 V, 4.2 mA, and 1 psi. That is a live zero. The listing accuracy is ±1 psi, so a 1 psi wander there is inside the sensor's tolerance. A pump or the house water is what shows a real change.

## ADS1115

October 6: one HiLetgo ADS1115 is on the A16 I2C header and answering. `kc868-a16-simple.yaml` reads it as `current_adc` at address `0x48`. The page names are ADC A0 through ADC A3, in volts, gain 4.096. Those are wiring checks. The current-sensor names are not in this firmware yet.

The header is P19, a 4-pin XH2.54 plug already soldered on the board. The cable plugs in. Do not solder the A16 end.

| A16 P19 | ADS1115 pin |
|---|---|
| 1 SDA | SDA |
| 2 SCL | SCL |
| 3 GND | GND |
| 4 3.3 V | VDD |

On the blue module the I2C row is VDD, GND, SCL, SDA, ADDR, ALRT. Match the names. Pin 1 on the A16 is not VDD on the module.

ADDR is jumpered to GND on the module. That is what selects `0x48`. ALRT stays open. VDD is the header's 3.3 V pin. The module's SDA and SCL pull-ups go to VDD, so 5 V, 12 V, or 24 V on that pin would pull the A16 I2C lines up with it.

October 6 jumper check passed. A channel tied to the module GND reads about 0 V. The same channel tied to the module VDD reads about 3.3 V. An open pin wanders and is not a failed channel. The current clamps are not connected. A0 is reserved for the pump, A1 for the tank, A2 for the UV, and A3 is spare. Those signals still need the bias circuit before they land on A0–A2. The A16 analog terminals are not the clamp inputs: A1 and A2 are 4–20 mA, and A3 and A4 are 0–5 V DC.
