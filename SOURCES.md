# Sources and earlier discussion

Initial references checked September 17, 2026; controller-model sources refreshed October 4, 2026.

## Project evidence

- **P17 — October 4 valve suppression decision:** Mike reports 3–5 ft solenoid wiring and declines splicing in valve-end flyback diodes. The connection schedule omits additional valve/panel diodes and retains the A16's onboard M7 protection shown in the manufacturer schematic. Earlier external 1N5408 advice remains a historical optional suggestion, not an active build requirement.

- **P16 — October 4 LED supply confirmation:** Mike confirmed all four switch/button LEDs use 12V DC. The connection schedule now supplies both output banks and board power from the existing 12V distribution; the pressure loop retains its 24V converter. Channel assignments are unchanged.

- **P15 — October 4 connection-planning direction:** Mike says the flush outlets are outside, ordered hardware and contactor ratings are settled, and CT/ADS assembly wiring is his work. He asks to decide what connects where before building the code. CONNECTIONS.md records the resulting channel assignment. Contactor suitability is accepted on Mike's statement, not reclassified as an independent test by this task.

- **P14 — October 4 current-sensor clarification:** Mike explicitly confirmed three current sensors for the initial build: hot-water tank, UV light and pump. This resolves their omission from the first assembly list; the existing one-ADS1115/three-conditioned-CT-channel plan remains the implementation baseline pending bench validation.

- **P13 — October 4 assembly/controller confirmation:** Mike selected one pressure sensor, no flow sensor, two 12V flush solenoids (one per function), illuminated pump/heater On / Off / Auto selectors and two illuminated flush buttons. Current device code and the HA package were read from the cabin configuration share without changes. His subsequent listing screenshot identifies the intended standard KC868-A16 and shows REV1.4. A listing image does not establish the physical unit's revision.

- **P01 — Current request:** Mike's equipment description and pasted 13-line cart, September 17. Source for current scope, candidate quantities and quoted prices. Item-level ownership and exact retailer links remain unknown.
- **P02 — Review cabin water system plan:** Codex task ID `01a0a1c9-f5b6-7352-89af-35d482ea7a2b`, September 14. Retrieved directly with the task reader on September 17, despite being outside this project. Initial plan and recent conversation establish the offline/ESPHome direction, HOA proposal, owned standard A16 boards, existing Sonoff experience, CircuitSetup preference, and exploration of separate ESP32/RS485 hardware.
- **P03 — Prior memory:** Cabin water-system memory entry and rollout summary helped locate P02 and flag previous complexity/overclaiming issues. Direct conversation retrieval controls where available. Prior assistant suggestions are not treated as user approvals or verified installations.
- **P04 — Draft cabin project reply:** Recent chat inspected to check for additional context. It concerns shelves, dock anchoring and soffit lights; no water-panel design decisions imported.
- **P05 — September 17 follow-up answers:** Mike confirmed 12V DC coils; 240V pump and heater with reported 20A each; 120V Cactus Pure UV; uncertain motor HP and flow units; pump/spin/triple-Big-Blue/UV/single-Big-Blue water order. Final housing's tannin purpose and cartridge labels remain tentative.
- **P06 — September 17 drain follow-up:** Both flush outlets discharge outside. Exact tee position relative to final housing was not unambiguously resolved by the reply.
- **P07 — September 17 scope/assumption direction:** Mike explicitly retained the tannin filter after UV, directed assuming the 20A figures are breaker sizes, described a likely 3/4HP or possibly 1HP 240V pump, and repeated 10 gph. These instructions supersede the earlier open relocation and breaker-meaning questions. Coil requirements remain 12V DC.

- **P08 — September 17 contactor selection:** Mike chose an additional 25A/12V DC contactor to replace the 16A/12V AC item, accepting the $5.00 difference at the pasted prices. His observation about brand availability was not independently verified and is not needed for this selection.

- **P09 — September 17 current detection and available parts:** Mike prefers CT clamps plus an ADC rather than CircuitSetup, reports a couple of 22mm pushbuttons from Ultra Lift and an owned 12-to-24V converter, and asks whether a second DIN supply would be cleaner. ADC architecture is now the working direction; converter reuse is an assistant recommendation, not a finalized supply selection.

- **P10 — September 17 operations/flow request:** Screenshot identifies DAE AS250U-100P at $99.99; Mike prefers vertical mounting and is open to alternatives. Follow-up confirms 1-inch copper and proposed push-to-connect adapters, but not upward/downward flow direction. Manufacturer comparisons and links are recorded in FLOW_OPTIONS.md.

- **P11 — September 17 ultrasonic question:** Mike asks how good ultrasonic flow sensors are. Current primary-source comparisons, limitations and the inline U-100b candidate are recorded in FLOW_OPTIONS.md; this is research, not a selection.

- **P12 — September 17 value/sizing discussion:** Mike leans toward deferring flow measurement, asks about 3/4-inch devices in 1-inch copper, and estimates approximately 6 gallons per minute after an initial per-hour wording. This is treated in context as a meter-sizing question; 6 gpm is a comparison assumption, not measured maximum flow. Pressure-monitoring limits and source-backed pressure-loss examples are in FLOW_OPTIONS.md.

## Technical references

October 4 connection bindings: [A16 ESP32 pin definitions](https://www.kincony.com/forum/showthread.php?tid=1666) gives I2C SDA GPIO4/SCL GPIO5 and analog A1 GPIO36. [A16 ESPHome bank reference](https://www.kincony.com/forum/showthread.php?tid=1628) supplies the baseline PCF bank addresses; the raw inversion flags in that older example are not adopted blindly. The [published schematic](https://www.kincony.com/download/KC868-A16-schematic.pdf) was visually inspected: its output banks provide positive supply to the loads, its PCF/optocoupler drive is active-low, and I2C header P19 is SDA/SCL/GND/3V at pins 1/2/3/4. An actual I/O check must bind these circuit expectations to the installed board. [Manufacturer address-variant support](https://www.kincony.com/forum/showthread.php?tid=2484) explains PCF8574A address substitutions. [ESPHome ADS1115 documentation](https://esphome.io/components/sensor/ads1115/) and [TI datasheet](https://www.ti.com/lit/ds/symlink/ads1115.pdf) support ADDR-to-GND at 0x48 and the chosen 3.3V module interface. This task assigns connections; it does not flash a device or change cabin configuration.

October 4 controller check: [KinCony hardware details](https://www.kincony.com/esp32-board-16-channel-relay-hardware.html) documents dry-contact inputs and MOSFET DC outputs; its older all-voltage analog description differs from later [manufacturer analog-input support](https://www.kincony.com/forum/showthread.php?tid=7890), which identifies A1/A2 for 4-20mA and A3/A4 for 0-5V. The [published schematic](https://www.kincony.com/download/KC868-A16-schematic.pdf) is dated February 3, 2025; it is a reference pending a physical-board revision/circuit match. Output-bank supply labels and pictured REV1.4 marking are from Mike's screenshot. Do not treat the analog description as a verified pin-level match to his unit yet.

| ID | Primary source | Use and limit |
|---|---|---|
| S01 | [KinCony: A16 analog-input support](https://www.kincony.com/forum/showthread.php?tid=7890) | Manufacturer support discusses 4–20mA and voltage inputs. Match the actual board/schematic before terminal assignment. |
| S02 | [KinCony: A16 wiring and supply revisions](https://kincony.com/forum/showthread.php?tid=3267) | Manufacturer notes supply changes between PCB revisions. Standard A16 remains the baseline; record revision for wiring. |
| S03 | [CircuitSetup six-channel meter documentation](https://github.com/CircuitSetup/Expandable-6-Channel-ESP32-Energy-Meter/blob/master/README.md) | Historical alternative only, superseded by P09. Its CT input constraints do not apply automatically to the new ADC interface. |
| S04 | [ESPHome CT clamp current sensor](https://esphome.io/components/sensor/ct_clamp/) | Current sensing requires a suitable sampling/interface arrangement and calibration; software support does not establish electrical compatibility with an A16 terminal. |
| S05 | [VIQUA FAQs](https://viqua.com/resources-24/faqs/) | Manufacturer explains monitored UV systems and treatment variables. General background only; subsequently identified Cactus reference below is more relevant. |
| S06 | [UV Pure Cactus owner's manual, Z800011G2](https://uvpure.com/Docs/Manuals/Cactus%20English%20Manual-Z800011G2.pdf) | December 2016 manufacturer manual; printed pp. 4-5 cover UV transmission and model ratings, p. 8 places UV after pretreatment, p. 11 describes anti-fouling flow. Exact installed model/revision still to match. |
| S07 | [ESPHome ADS1115 documentation](https://esphome.io/components/sensor/ads1115/) | I2C ADC support, continuous mode for CT use, input limits and configuration. Does not establish a validated multi-CT sampling schedule. |
| S08 | [OpenEnergyMonitor CT introduction](https://docs.openenergymonitor.org/electricity-monitoring/ct-sensors/introduction.html) | CT voltage/current-output distinction and interface principles. Actual SazkJere product not independently authenticated as YHDC. |
| S09 | [Texas Instruments ADS1115](https://www.ti.com/product/ADS1115) | Four single-ended or two differential inputs, multiplexed converter, up to 860 samples/second. Primary ADC specifications. |
| S10 | [WIKA A-10 pressure transmitter](https://shop.wika.com/en-us/a_10.WIKA?ProductGroup=100277&fromGeoIpOverlay=true) | Example 4-20mA device with 8-30V DC supply; illustrates why actual sensor/loop specifications matter. Not Mike's identified sensor or a purchase recommendation. |

## Corrections to the earlier draft

- Mike subsequently confirmed pump/heater/UV supply voltages. Flush durations, low-pressure thresholds and inactivity intervals from the prior draft remain unverified, not commissioning settings.
- A selector's Off position is an operating control, not proof of electrical isolation.
- Low water pressure is not a reliable tank-full signal for heater dry-fire protection.
- Turning off the pump cannot isolate water already stored under pressure.
- UV electrical draw cannot by itself establish treatment performance.
- The meter/ESP32/RS485 proposal has not been bench-tested. Previous pin assignments have not been copied into the build documents.
- A straight two-wire 240V load carries the same current in each hot conductor: report actual conductor amperes, not doubled amperes. Any power scaling depends on the voltage-reference arrangement and must be explicitly validated.
- No availability claims from the September 14 conversation or shipping promises in the September 17 cart are carried forward as verified current information.

## Still to obtain

Exact pump/heater/UV manuals and nameplates; exact A16 revision schematic; contactor part numbers and load/coil data; valve pressure/flow/coil data; enclosure usable dimensions and environmental specifications; terminal/wire approvals and ratings; existing ESPHome configuration; retailer links and item-level stock.
