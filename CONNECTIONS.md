# Water closet connection schedule

October 6 bench check, recorded in [WIRING.md](WIRING.md). The selector inputs below are the terminals that answered on the live board. The output list is the four LED outputs and the three contactor outputs, plus the two flush valves.

`kc868-a16-simple.yaml` is the page that was used for the check. That page drives the four LEDs from the selectors and buttons. It leaves the three contactors as manual switches. The consolidated `water-closet.yaml` was flashed over OTA on October 6; its bindings match this schedule. Facing the knob, left is On and right is Auto.

Updated October 6, 2026. This file owns terminal assignments. Behavior and timings remain in DESIGN.md and DOOR_CONTROLS.md.

Mike has settled the ordered hardware, contactor selection and load ratings. Both drains discharge outside. Those are accepted build inputs, not outstanding procurement checks. Mike owns wiring the CT signal-conditioning/ADS assembly. This schedule defines its connection to the controller and the channel order.

## Channel assignment

Use the numbered A16 terminal labels, not connector position counted from an edge. The two output connectors are physically arranged in different directions in the supplied image. IN/OUT numbers below are A16 channels, not ESP32 GPIO numbers.

### Door contacts

| A16 input | What the bench found | Simple-page ID | PCF8574 input bank 1 |
|---|---|---|---:|
| IN1 | Hot water tank, On (left) | `tank_left` | 0 |
| IN2 | Hot water tank, Auto (right) | `tank_right` | 1 |
| IN3 | Water pump, On (left) | `pump_left` | 2 |
| IN4 | Water pump, Auto (right) | `pump_right` | 3 |
| IN5 | Spin filter flush button | `spin_filter_flush` | 4 |
| IN6 | UV button | `uv_button` | 5 |
| IN7–IN16 | Not connected | — | — |

Facing the knob, left is On, center is Off, and right is Auto. Center leaves both of that selector's inputs open. Red and black on each selector are the LED pair, not the position contacts.

The contact closes an A16 input to input ground. On this board a closed contact reads `true` with `inverted: true`. The October 6 page used addresses `0x21` and `0x22` for the two input banks.

The operational dashboard also reads unconnected IN7–IN16 for bench testing. Reading those spare terminals does not assign them an equipment function. Its test panel labels contacts Closed/Open and identifies the terminal number.

### LED outputs

| A16 output | Connection | Simple-page ID | Operational firmware ID | PCF8574 output bank 2 pin |
|---|---|---|---|---:|
| OUT9 | Pump selector LED | `pump_output` | `pump_indicator` | 0 |
| OUT10 | Hot water tank LED | `tank_led` | `tank_indicator` | 1 |
| OUT11 | Spin filter flush LED | `filter_flush_led` | `spin_flush_indicator` | 2 |
| OUT12 | UV button LED | `spin_flush` | `system_flush_indicator` | 3 |

On the simple page, tank On or Auto turns OUT10 on, and both open turns it off. Pump On or Auto does the same with OUT9. The spin filter flush button toggles OUT11. The UV button toggles OUT12. Each press leaves that LED where it is. Output banks answered at `0x24` and `0x25`, with `inverted: true` turning a load on.

The simple-page IDs are historical names: `pump_output` and `spin_flush` above both drive LEDs. In the operational firmware, IN6 requests the system/UV flush through OUT5; OUT12 indicates that flush. The UV button does not directly toggle UV lamp power. Lamp power uses OUT3 and the controller's UV timing policy.

All four LEDs are 12 V DC. Each LED positive comes from its output. Each LED negative goes to DC 0V. Output bank 2 uses the same 12 V distribution as the board. The 12-to-24 V converter remains the pressure-loop supply. Red and black on each selector are the LED pair, not the position contacts.

### Contactor outputs

| A16 output | Connection | Firmware ID | PCF8574 output bank 1 pin |
|---|---|---|---:|
| OUT1 | Pump contactor coil | `pump_contactor` | 0 |
| OUT2 | Hot water tank contactor coil | `tank_contactor` | 1 |
| OUT3 | UV contactor coil | `uv_contactor` | 2 |

The simple page does not operate these from the selectors. They remain manual switches there. Each coil negative returns to DC 0V. The A16 output supplies the switched positive.

### Flush valves

| A16 output | Connection | Firmware ID | PCF8574 output bank 1 pin |
|---|---|---|---:|
| OUT4 | Spin-flush valve | `spin_flush_valve` | 3 |
| OUT5 | System-flush valve | `system_flush_valve` | 4 |
| OUT6–OUT8, OUT13–OUT16 | Not assigned | — | — |

Valve suppression decision, October 4: Mike reports approximately 3–5 ft of wire to each solenoid and does not want to cut/splice the valve leads to install local diodes. Omit the external valve-end 1N5408 diodes previously suggested; retain the intact valve wiring and rely on the A16's onboard M7 flyback protection shown in the manufacturer schematic. No added diode at the valve or panel is planned for these two circuits. This decision concerns the flush solenoids; it does not change the selected contactors' existing coil suppression.

### Pressure

| Connection | Assignment |
|---|---|
| A16 analog A1 | Pressure receiver input |
| ESP32 pin | GPIO36 |
| Firmware ID | `water_pressure` |
| Sensor supply | Owned 12-to-24V converter; pressure loop uses +24V |
| A16 analog A2–A4 | Unassigned |

Use the owned LEFOO 0–100 PSI, 4–20mA sensor. Red goes to converter +24 V, black to A16 A1, and the bare shield to analog GND only. Converter negative goes to analog GND. The 24 V supply does not land on A1. October 6: A1 to the 12 V negative measured 148.8 Ω, and the open-air loop read about 0.63 V, 4.2 mA, and 1 psi. Wire colors and the live-zero reading are in WIRING.md.

### Current sensors and ADS1115

| ADS1115 analog input | Conditioned signal from | Candidate firmware IDs (ADC / uncalibrated CT signal) |
|---|---|---|
| A0 / AIN0 | Pump CT | `pump_adc` / `pump_ct_raw` |
| A1 / AIN1 | Hot-water-tank CT | `tank_adc` / `tank_ct_raw` |
| A2 / AIN2 | UV CT | `uv_adc` / `uv_ct_raw` |
| A3 / AIN3 | Unassigned; no pressure reading on this ADC | — |

These are **ADS channels**, separate from the A16 analog A1 used for pressure. Mike supplies the conditioned CT signals. Each CT surrounds one outgoing load conductor, with its conditioning output referenced to ADS GND. For the two-wire 240V loads, use one hot conductor per CT; the UV CT uses the outgoing line conductor. CT orientation can be consistent for all three.

| ADS connection | A16 connection / setting |
|---|---|
| VDD/VCC | A16 3.3V logic supply |
| GND | A16 GND / shared signal return |
| SDA | A16 I2C SDA, ESP32 GPIO4 |
| SCL | A16 I2C SCL, ESP32 GPIO5 |
| ADDR | GND, selecting address `0x48` |
| ALERT/RDY | Unconnected for this baseline |

Use the labeled I2C header connections. The published schematic identifies header P19, part XH2.54-4P: pin 1 SDA, pin 2 SCL, pin 3 GND, pin 4 3V. The controller's 3V label is its 3.3V logic rail. Keep module pullups on that logic rail. ADS power and signal pins do not use 12V or 24V.

October 6 bench check: the owned HiLetgo module is plugged into P19 with ADDR tied to GND. It answers at `0x48`, and A0–A3 read about 0 V jumpered to module GND and about 3.3 V jumpered to module VDD. See WIRING.md. The current clamps and their bias circuit are not connected yet.

One ADS1115 serves all three current channels. The candidate acquires the channels sequentially so one CT sampling window owns the ADC multiplexer at a time. Sampling defaults off until the conditioning is connected. The candidate reports uncalibrated AC signal volts; calibrated amperes remain unfinished. The channel assignments do not depend on final sampling timing. Preserve separate measured-current and contactor-enabled states.

## Power connection summary

| Supply destination | Feed |
|---|---|
| A16 board power input | +12V and DC 0V |
| Output bank 1 supply, OUT1–OUT8 | +12V at its bank supply input |
| Output bank 2 supply, OUT9–OUT16 | +12V at its bank supply input; all four LEDs confirmed 12V DC |
| All coil/valve/LED returns | DC 0V distribution |
| Pressure converter input | Existing +12V/DC 0V distribution |
| Pressure transmitter loop | Converter +24V; common return as above |
| ADS/CT conditioning logic supply | 3.3V/GND interface, as assigned above |

Board power and both bank feeds are distinct terminals. A16 bank common feeds are positive supply connections, not spare outputs or grounds. Use the labels on the board rather than the appearance of a connector. DC 0V and protective earth are separate connections; this schedule does not introduce a bond between them.

## Firmware mapping reference

The manufacturer's standard A16 mapping gives:

| Interface | Expected setting |
|---|---|
| I2C bus | SDA GPIO4, SCL GPIO5 |
| `inputs_1_8` | PCF8574 `0x21`, local pins 0–7 |
| `inputs_9_16` | PCF8574 `0x22`, local pins 0–7 |
| `outputs_1_8` | PCF8574 `0x24`, local pins 0–7 |
| `outputs_9_16` | PCF8574 `0x25`, local pins 0–7 |
| `current_adc` | ADS1115 `0x48` |
| Pressure | ESP32 GPIO36 |

The schematic's optocoupler/MOSFET circuit indicates active-low output-expander drive: PCF LOW turns the positive load feed ON. The October 6 board confirmed that polarity. Input banks `0x21` and `0x22`, and output banks `0x24` and `0x25`, answered with `inverted: true`.

If Ethernet is used, retain its onboard LAN8720 configuration: MDC GPIO23, MDIO GPIO18, clock GPIO17 output, PHY address 0. These existing board functions do not consume any assigned door-control channels. Network addressing is a firmware setting to select later.

## Connection review before operational code

- Door inputs IN1–IN6 are the October 6 bench map above. Facing the knob, left is On, center is Off, and right is Auto.
- Four LED outputs are OUT9 pump, OUT10 tank, OUT11 spin filter flush, and OUT12 UV button. Three contactor outputs are OUT1 pump, OUT2 tank, and OUT3 UV. Flush valves are OUT4 and OUT5.
- ADS `0x48` answers on P19. Verify the three CT channel identities once the clamps and bias circuit are connected; determine sampling/calibration during firmware work.
- A1 is the 150 Ω current input. The powered open-air LEFOO reading was about 0.63 V, 4.2 mA, and 1 psi. A known water pressure is still the calibration check.
- LED bank voltage is settled at 12V DC. Board power and both output banks share the single 12V supply, with a separate feed connection to each. Contactor purchasing and outdoor drains are settled.

## Source references

Channel/function allocation is this project's October 4 design choice. Hardware bindings come from [KinCony A16 pin definitions](https://www.kincony.com/forum/showthread.php?tid=1666), [standard expander-bank mapping](https://www.kincony.com/forum/showthread.php?tid=1628), and the [manufacturer schematic](https://www.kincony.com/download/KC868-A16-schematic.pdf), inspected visually.

Other supporting sources: [KinCony dry-contact/current-input explanation](https://www.kincony.com/forum/showthread.php?tid=6539), [address variants](https://www.kincony.com/forum/showthread.php?tid=2484), [ESPHome ADS1115 addressing and CT mode](https://esphome.io/components/sensor/ads1115/), and [Texas Instruments ADS1115 datasheet](https://www.ti.com/lit/ds/symlink/ads1115.pdf). The October 6 bench check covers the I2C header, address `0x48`, and DC voltage reads. CT waveform sampling is still untested.
