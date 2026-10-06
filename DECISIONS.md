# Decisions and next steps

Updated: October 6, 2026.

## Decision record

October 6 operating decisions supersede older modes/frequencies in the historical record below:

- **D53 — Power restoration and heater startup:** Mike approved restoring the saved Normal/Away/Shutdown mode with outputs initially off, immediately committing infrequent mode changes to flash and a short pump-before-heater delay. Implement five seconds after every pump enable, canceled by loss of permission, for Auto and On heater requests. Slow tank blink and dashboard status explain the wait. This does not detect a full tank; filling remains an operator responsibility. Document, but do not implement before CT installation/calibration, a future Normal startup gate requiring an observed pump run followed by a stop. A pump cycle does not prove the tank is full. See DESIGN.md for restoration, interrupted-run and commissioning rules.
- **D52 — Dormant UV electrical fault and usage monitoring:** Mike requested low-current UV fault code and pump/tank runtime/cycle statistics before CT installation. Add explicit monitoring enable, calibration and thresholds; absent sensors remain unknown. UV gets a fresh warmup grace per energization and sustained-low-current confirmation, with a rapid 5 Hz fault indication taking priority on its LED. No new equipment trip. Runtime and detected starts are informational estimates since restart; ten-second sampling can miss short cycles. See DESIGN.md for defaults, limits and the short fault list.
- **D51 — Independent Away water-flush schedules:** Mike confirmed Away UV means water exchange, leaving the UV lamp policy unchanged. Add independently configurable spin enable/frequency/duration and show both schedules, next-run estimates and paused/disabled states at the top. New Away spin defaults disabled, one/day, 30 seconds. Existing system settings are retained. Due flushes serialize, spin first; each temporarily enables the Auto pump with heater/UV inhibited. Spin LED now stays on for an enabled automatic spin schedule and fast-blinks while flushing, superseding D50's earlier steady-during-flush rule. HA connectivity is a separate informational dashboard status; it does not interrupt autonomous control.
- **D50 — Everyday dashboard and door feedback:** Mike confirmed the wiring through the bench UI and approved putting mode/status first, with test controls kept under collapsed Advanced. Show the actual UV minimum-off countdown. Moving both selectors to Auto requests Normal; unchanged Auto positions preserve explicit Away/Shutdown. UV button LED: slow blink pending start, steady enabled, fast blink system/Away flush. Spin LED is steady during spin flushing. Wi-Fi loss uses two 200 ms flashes together on both button LEDs every five seconds while neither valve is active; active flushing takes priority. Pump/tank indication and the pump-disabled/heater-disabled interlock continue as before. Detailed timings and priority are in DESIGN.md. This supersedes D46's earlier generic shared offline blink.
- **D49 — Dashboard and direct bench testing:** Mike requested mode control at the top of the local page and a test box with direct output toggles and live input indicators. Normal/Away web/API selection requires both selectors Auto; Shutdown always works. An explicit bench session suspends normal arbitration and permits independent OUT1–16 tests, while IN1–16 stay observable. All Outputs Off clears commands, and leaving testing returns to Shutdown with all outputs off. The operating interlocks remain in force outside bench testing. Sessions do not restore after reboot; missing sensor readings remain expected at this installation stage.
- **D48 — Sensor installation stage:** After the first A16 flash, Mike confirmed equipment/sensor installation is incomplete and the current sensors are not installed. Zero/unavailable pressure and missing current measurements are expected bench conditions; no troubleshooting is requested for them now. Keep CT sampling off. UV switching/anti-cycling remain usable, while the Sonoff's electrical-health monitoring awaits CT installation, calibration and corresponding local logic.
- **D42 — Local modes:** Mike ultimately selected **Normal, Away and Shutdown**, dropping Maintenance after reviewing its use case. Remove Winter and the old combined Shutdown/Maintenance mode. Shutdown commands everything off; selectors, timed flush buttons and the web UV enable setting provide manual controls. All equipment coordination belongs on the A16. HA integration is later work.
- **D43 — Flush purposes/frequency:** Spin flushing once daily is sufficient as a starting point. Retain a short Normal-mode UV refresh and add a distinct longer Away exchange through the four Big Blue housings. Away frequency is adjustable from one to four runs per day, with runtime calibrated at the actual drain. A rough 6–7-gallon system estimate suggests 12–20 gallons for two to three exchanges; 15 gallons per run is the proposed starting target; do not treat old defaults as approved settings.
- **D44 — Water source and operating problem:** Mike reports lake water and substantial buildup/stale water in the Big Blue housings after absence, currently requiring 15–20 minutes of running water on return. This is user-reported operating experience; the new exchange schedule's effectiveness has not yet been measured.
- **D45 — Pump/heater interlock:** Mike explicitly requires heater disable whenever the pump is disabled, including tank selector On with pump selector Off. The candidate already enforces this in all three modes. This means pump command enabled, not motor drawing current; ordinary pressure-switch idle is not pump disable.
- **D47 — Selector order:** Facing the knob, left is On, center is Off, and right is Auto. On is the Hand position. Both the pump and tank selectors use this order. The bench left ends are IN1 (tank) and IN3 (pump); the right ends are IN2 and IN4.
- **D46 — Simple local control and indication:** Mike prioritizes local Normal/off and a useful indication of network loss. The candidate uses both selectors Off for Shutdown and deliberate movement to On/Auto for Normal; pump Auto + tank Off does not imply Away. Away remains an explicit web/API setting. Initial Auto readings after boot do not cancel a saved Away mode. Tank LED blinks for requested heat blocked by pump disable, without faulting intentional Away/Shutdown off states. Both flush LEDs blink together on Wi-Fi loss while no valve is open; active-flush indication has priority. UV anti-cycling is required; the candidate applies a five-minute minimum-off time. See DESIGN.md for the current working behavior.
- **Implementation checkpoint:** `water-closet.yaml` and supporting controller/packages were compiled and flashed over OTA on October 6 at Mike's request. Its bindings match the clarified CONNECTIONS.md schedule: OUT1–3 contactors, OUT4–5 valves, OUT9–12 LEDs. Post-reboot API verification confirmed the new firmware in Shutdown with both selectors Off, equipment commands off, and no selector/input-communication fault. Pressure reported invalid. No Pi configuration was changed. Current behavior and proposed choices are at the top of DESIGN.md; physical commissioning remains outstanding.

| ID | Topic | Current position | Basis/status |
|---|---|---|---|
| D01 | One control enclosure | Project objective | Confirmed September 17 |
| D02 | Main controller | Standard KinCony KC868-A16 selected; listing screenshot shows REV1.4 | October 4 user selection. MOSFET DC outputs, separate board/output-bank supplies. Confirm revision on the physical unit and match schematic before final wiring. |
| D03 | Firmware/local operation | A16 owns all sequencing, interlocks, timing and flushing; HA supplies display/history/settings/requests | Mike explicitly confirmed local arbitration. Retire/redirect HA per-device enforcement during migration. |
| D04 | Manual pump/heater operation | Input-only On / Off / Auto selectors | D39 supersedes hardware bypass. Two ground-switched NO inputs per selector; A16 operates coil. On requires working controller. |
| D05 | Current detection | Initial build includes three CTs: hot-water tank, UV light and pump; conditioning into one owned HiLetgo ADS1115 remains the baseline | Mike explicitly reconfirmed all three October 4. Use three of four inputs; validate non-overlapping waveform sampling, UV sensitivity and calibration. Three-ADC suggestion is fallback only. Scope is amps/running detection. |
| D06 | Meter host/data link | A16 reads the owned HiLetgo ADS1115 over the local I2C header at `0x48` | October 6 bench check confirmed P19 and DC voltage reads on A0–A3. No separate metering ESP32/RS485 bridge. CT conditioning and waveform sampling are not yet validated. |
| D07 | Contactors and coils | Three selected 25A, 2-pole 2NO contactors with 12V DC coils, allocated to pump/tank/UV | Mike confirms ordered hardware and adequate contactor ratings October 4. Accept as build inputs; no further selection/rating review gates the channel schedule. |
| D08 | Flow/pressure additions | One pressure sensor; no flow sensor in the initial build | Reconfirmed October 4; existing single-LEFOO selection remains in D30/D33 |
| D09 | UV monitoring terminology | Electrical operation and UV treatment performance are different measurements | Design correction; see S05 |
| D10 | Ordering/building | Hardware ordering underway; prepare for assembly | Mike reports much of the hardware ordered October 4; item-level ordered/received inventory not yet reconciled |
| D11 | Equipment supplies | Pump/heater 240V; UV 120V; assume 20A pump/heater breakers | Mike directed the breaker assumption; not measured operating current |
| D12 | Existing water order | Pump -> spin filter -> triple Big Blue -> UV -> single Big Blue | User-reported; cartridge identities, pressure tank and flush tee unresolved |
| D13 | Flush discharge | Both outlets discharge outside | Mike confirmed September 17 |
| D14 | Tannin filter position | Retain after UV; no relocation in this project | Mike's explicit direction September 17; closed scope decision |
| D15 | Pump sizing basis | Likely 3/4HP; use up to 1HP at 240V for planning | User-directed assumption; retain candidate 25A contactors subject to exact-model motor rating |
| D16 | Manual flush controls | Two illuminated pushbuttons: spin flush and UV/system flush | Selected October 4; all requests run through A16 |
| D17 | Pressure power | Use owned 12-to-24V converter as the working plan | Mike reports previous attempt with 12V from A16 produced no reading and prior research pointed to 24V. Do not repeat a 12V-first plan. Successful 24V operation not yet reported; verify loop wiring/input and measured current during commissioning. |
| D18 | Flow-meter location | 1-inch copper; vertical mounting preferred; push-to-connect adapters envisaged | Mike confirmed pipe size; vertical flow direction remains unspecified |
| D19 | Flow-meter scope | Deferred from the first build | Mike agreed; preserve ability to add later, no meter purchase planned |
| D20 | Ultrasonic flow option | Investigate inline DAE U-100b alongside clamp-on alternatives | User asked about performance; assistant shortlist only. Verify response/low-flow behavior and Modbus data before selection. |
| D21 | Flow comparison/sizing | Approximately 6 gpm; consider 3/4-inch meters in 1-inch copper | Current user estimate, not measured maximum; assess actual meter/fitting pressure loss |
| D22 | Enclosure | Existing horizontal-divider case versus larger replacement under consideration | Latest screenshot purchase banner indicates 13 x 9.2 x 5.6-inch owned variant; selected 18.1 x 12.6 x 6.4-inch option at $75.99 conflicts with title. Earlier B12 ownership mapping uncertain; verify physical dimensions and usable layout. |
| D23 | Low-voltage distribution | Existing DIN distribution hardware available; recommend reuse if suitable | Exact device not identified; do not assume it is the ANMBEST cart item. Branch protection still required as designed. |
| D24 | Ground/neutral bars | Mike now prefers DIN-mounted ground and neutral blocks; exact products pending | Supersedes backplate-bar layout. Add actual block widths to rail estimate; neutral insulated from rail/earth. Confirm aluminum-rail compatibility if PE terminals use rail as grounding path. |
| D25 | UV cooling trigger | Periodic timed flushes; omit temperature sensor from initial build | Mike prefers simple frequent refresh; interval/duration and effective cooling remain to establish |
| D26 | Leak detection | Optional simple alarm; no automatic shutdown planned | User receptive but not committed; under-cabin location limits perceived consequence, actual water-collection location remains unknown |
| D27 | Room temperature | Retain existing Sonoff thermostat; prefer reusing its reading over another room sensor | User reports working room-temperature control. Model/integration unknown. Cabinet-temperature monitoring is a separate optional function, not selected. |
| D28 | AC field wiring | Incoming and outgoing AC cables land on terminal blocks; ten hot-conductor positions fixed by Mike | Pump 4, heater 4, UV 2; no spare positions added. Resolve supply branch within that arrangement using suitable terminal/splice; neutral and earth counted separately. |
| D29 | 120V source and UV output | Same incoming 120V circuit powers control supply and switched UV | Mike proposes male/female cut-cord leads; physical cord assembly suitability remains to check. Control supply branches before UV contactor, remains powered when UV is off. Two 240V sources remain live when 120V is unplugged. |
| D30 | Pressure transmitter inventory | One LEFOO T2000, 0-100 PSI, 4-20mA, G1/4 male | User bought one to validate first. No second purchase selected; test readout against gauge before committing to a matched upstream/downstream pair. |
| D31 | Pressure test history | Only 12V/A16 with mouth-blown low pressure tested; never 24V or normal 30-50 PSI | Inconclusive result. Test 24V loop current/readout at zero and known pressure before declaring sensor failure. |
| D32 | Winter pressure-sensor requirement | Survive subfreezing storage after compressed-air winterization | Retained water in sensor cavity is a distinct risk from cold ambient. Investigate explicit freeze-proof option (Ashcroft XUP candidate) or accepted removal/drainage procedure; no replacement selected. |
| D33 | Pressure-sensor spending decision | Proceed with the one owned LEFOO; drop Ashcroft from active selection | Mike rejects the cost of the Ashcroft route. Next step is a 24V test with known pressure; no replacement or second sensor purchase planned now. Winter survival remains unverified. This supersedes the active Ashcroft investigation in D32. |
| D34 | ADC assembly mounting | Add one MECCANIXITY DIN carrier for 100 x 72mm PCB | User screenshot/request; interface board holds one owned ADS1115 and three CT conditioning circuits. Actual carrier footprint and whether board is included remain to check. |
| D35 | Local interface | Two illuminated pump/heater On / Off / Auto selectors and two illuminated flush buttons; no display | Six digital inputs. UV flush is not a lamp-power toggle. LED meanings settled in D50. |
| D36 | UV anti-cycling history | Five-minute protection is the proposed A16 policy, not the verified existing setting | Read-only inspection found 60-second HA mode delays and an immediate Sonoff button toggle; an older guide contained a stale five-minute statement. The local timing policy is the five-minute minimum-off in DOOR_CONTROLS.md. |
| D37 | Winter remote-start prevention | Use existing breakers/disconnects for positive seasonal shutdown | D39 supersedes selector-based hardware interruption. Door Off and software Winter are not isolation. Map/label actual disconnects in operator procedure. |
| D38 | Control-power failure recovery | One installed supply; failure is a repair/replacement event | Earlier backup connector, dual-supply redundancy and mechanical override proposals are superseded by D39. Service spare optional, no failover hardware planned. |
| D39 | Simplicity and accepted downtime | One A16, one supply, input-only selectors; no controller-independent manual operation requirement | Mike accepted September 17. Seasonal cabin use approximately 60 days/year; prefer understandable operation and repair over hypothetical-failure contingencies. See rationale below. |
| D40 | Initial flush hardware and coverage | Two 12V solenoid valves, one per flush: spin and downstream system/UV | October 4 user confirmation. System/UV flush serves the cold-water treatment path before the house; actual takeoff and effective coverage still need physical verification. |
| D41 | Connection schedule | Four LEDs OUT9–12; three contactors OUT1–3; valves OUT4–5; tank IN1/2; pump IN3/4; buttons IN5/6 | October 6. LEDs: pump, tank, spin filter flush, UV button. Contactors: pump, tank, UV. ADS `0x48` on P19. Pressure A1 is the 150 Ω input. |

## Governing simplicity decision — D39

Additional build decision, October 4: omit external flyback diodes on the two flush valves. Mike reports 3–5 ft valve leads and prefers to leave them intact; use the A16's onboard M7 flyback protection. CONNECTIONS.md owns this wiring choice. The earlier external 1N5408 suggestion is not an active purchase or installation requirement.

Mike explicitly accepted controller-based On / Off / Auto operation and a single control supply. A controller or supply failure is an accepted repair event. No quantified failure odds are available; seasonal use does not establish a reliability percentage. This is a deliberate cost/complexity tradeoff for this cabin, not a claim that failure cannot happen.

Remove hardwired selector bypasses, dual supplies, ORing/redundancy modules, external backup-supply transfer and mechanical contactor overrides from active work. Keep local operation independent of Home Assistant, existing OEM protections, and ordinary electrical protection. Use breakers/disconnects for seasonal shutdown. The October 4 selection of flush pushbuttons preserves this architecture.

Do not reopen contingency features merely because another hypothetical failure is suggested. Revisit only at Mike's request or after actual operating experience establishes a concrete need. D39 supersedes earlier bypass/recovery proposals in D04, D16, D35, D37 and D38.

## Questions, in useful order

October 4: earlier ordering, contactor rating and outside-drain questions in this table are closed for the current work. LED supply voltage is confirmed 12V DC. CONNECTIONS.md is the active connection owner. Next record interface/address/polarity checks and finalize operating behavior for the local firmware.

| Priority | Question | Why it matters |
|---|---|---|
| Before ordering | Verify exact candidate contactor's motor rating for up to 1HP at 240V using assumed 20A circuits. | Finish load compatibility without repeatedly requesting nameplates during preliminary planning |
| First | Which cart items and metering parts are already in stock? | Prevent duplicate purchases |
| First | Pin down whether the flush tee is after the final Big Blue housing; both outlets are confirmed outside. Then record pressure-tank position and cartridge labels. | Sensor placement and true flush coverage |
| Next | Is the original anti-fouling kit present, and how does the current flush operate? Tannin filter location stays unchanged. | Cooling design |
| Next | What currently triggers each flush, how long does it run, and what isn't working well? | Preserve useful behavior and solve the actual deficiencies |
| Next | Which existing monitor reads UV power and what readings indicate normal operation today? | Reuse and a meaningful baseline |
| Before ADC purchase | Validate CT conditioning, ADC/A16 interface and multi-channel sampling; test UV sensitivity. | Implement current/running detection without unnecessary energy-meter hardware |
| Before pressure wiring | Identify sensor supply/output and existing converter model; check mounting and loop voltage budget. | Decide converter reuse versus direct 12V operation or separate DIN supply |
| Deferred | Revisit gallon tracking only if later needed | Flow meter excluded from initial build |
| Before flow-sensor selection | Use 6 gpm for comparison; establish peak flow and UV capacity if selecting a meter. | Actual flow remains unverified; size by pressure loss and low-flow performance |
| Next | Is the third contactor intended for UV? | Define its load and suitable coil/rating |
| Bench | Observe concurrent-flush handling and LED meanings on the door. | One valve at a time; same button cancels, other ignored. D50 defines the four LED indications. |
| Before layout | Where is the enclosure mounted, what space is available, and is there condensation, splash or freezing? | Enclosure material, rating, dimensions and thermal design |
| Before layout | What feeds, breakers, disconnects and grounding arrangements are available at the panel? | Mains layout and installer review |
| Before automation | Finish low-pressure/leak response after sensors are installed; outage restoration is settled by D53 and UV electrical indication by D52. | Commission thresholds and responses against real equipment |
| Before automation | Is the cabin seasonally drained or kept heated, and what must remain active when vacant? | Winterization and unattended operation |
| Before configuration | Is Ethernet available; where does Home Assistant run; what current ESPHome configurations can be reused? | Network setup without moving essential controls out of the panel |

Partial answers are sufficient; answer the first group before diving into every later detail.

## Work stages

| Stage | Deliverable | Completion condition |
|---|---|---|
| 1. Survey | Equipment/stock table and actual water-path sketch | Models, feeds, existing controls and both drain routes recorded |
| 2. Design | Functional diagram, behavior table and logical I/O map | Control priorities and monitoring scope settled |
| 3. Select | Exact BOM, load budget and scaled enclosure layout | Ratings/interfaces verified, parts physically fit, remaining buy quantities known |
| 4. Build preparation | Wiring drawing, terminal schedule, labels and firmware configuration | Specific board/parts matched; mains design reviewed by responsible installer |
| 5. Bench build | Mounted low-voltage controls and tested local behavior | Fault/restart/timeout checks passed before field load operation |
| 6. Installation | Completed panel and documented plumbing/electrical connections | Installation inspected as applicable; each equipment control verified |
| 7. Commissioning | Baselines, settings and simple operator sheet | Function/fault checks complete; configurations and as-built records saved |

## Planned commissioning checks

- Verify ground-contact position decoding, software Off priority over remote requests, and On/Auto operation through A16. Controller-independent Hand operation is explicitly not required (D39).
- Verify every output's inactive/startup state, valve timeout and no interrupted-flush replay.
- Test loss of network/Home Assistant, restart, power loss, and invalid/stale sensor readings.
- Cut power immediately after each mode change, especially Shutdown, and restore with both selectors Auto. Verify the saved mode, no flush/test replay, five-second heater delay and fresh UV minimum-off delay. Observe that pump Off or Shutdown cancels pending heat immediately.
- Verify each valve's real flow path, drain handling and closure; record purge effectiveness and cooling behavior.
- Calibrate flow/pressure readings against a reference; establish pressure drop at known flow.
- Compare current/power readings with reference instruments at realistic loads, including the small UV load; distinguish normal thermostat/pressure-switch idle from failure.
- Establish UV startup and steady-state electrical baselines and test available manufacturer alarm signals.
- Finalize alarm response, winterization, maintenance isolation and post-service restoration instructions.

## Change log

- 2026-10-06 selector order: Mike adopted the usual Hand-Off-Auto layout. Facing the knob, left is On, center is Off, and right is Auto.

- 2026-10-06 output list: Four LED outputs are OUT9 pump, OUT10 tank, OUT11 spin filter flush, and OUT12 UV button. Three contactor outputs are OUT1 pump, OUT2 tank, and OUT3 UV. Flush valves remain OUT4 and OUT5.

- 2026-10-06 installed wiring: The live-board check is the connection schedule. Tank selector IN1/2, pump selector IN3/4, spin button IN5, UV button IN6. Four LEDs are OUT9–OUT12. Three contactors are OUT1–OUT3. Expanders answered at `0x21`, `0x22`, `0x24`, and `0x25` with `inverted: true`.

- 2026-10-06 ADS1115 bench check: The owned HiLetgo module is on A16 header P19 (XH2.54-4P). ADDR is tied to GND, address `0x48`. `kc868-a16-simple.yaml` shows A0–A3 as volts. A GND jumper reads about 0 V and a VDD jumper reads about 3.3 V. Current clamps are not connected. Details are in WIRING.md.

- 2026-10-04 valve suppression: Mike elected to omit external valve-end diodes for the approximately 3–5 ft solenoid runs. Record reliance on the A16's onboard M7 flyback diodes and leave valve leads intact; no additional panel diode is assigned.

- 2026-10-04 LED supply confirmation: Mike confirmed all four switch/button LEDs are 12V DC. Assigned the same 12V distribution to both output banks and board power; pressure converter remains dedicated to the 24V pressure loop. No channel assignments changed.

- 2026-10-04 connection planning: Accepted Mike's hardware/contactors/ratings and outside-drain direction. Created CONNECTIONS.md with six door inputs, five equipment commands, four independent LED outputs, pressure A1/GPIO36 and three ADS current channels. Assigned the three contactors to pump/tank/UV. CT conditioning/ADS assembly belongs to Mike; firmware pin bindings are sourced from manufacturer documentation and schematic. No live configuration changed.

- 2026-10-04 current-sensor scope confirmation: Mike added all three current sensors to the initial assembly scope: hot-water tank, UV light and pump. Retained one owned ADS1115 with three conditioned CT channels and local I2C connection to A16. Corrected stale active-design paragraphs recommending three ADCs; additional ADCs remain a fallback only if bench validation requires them.

- 2026-10-04 controller selection: Mike supplied the standard KC868-A16 listing screenshot, marked REV1.4. Recorded intended model, separate controller/output-bank power connections and MOSFET output architecture. Physical PCB revision and pressure-channel circuitry remain to verify before terminal-level wiring; no hardware or live firmware changed.

- 2026-10-04: Recorded hardware ordering and initial assembly scope: one pressure sensor, no flow sensor, two 12V flush valves (one per function), pump/heater illuminated On / Off / Auto selectors, and two illuminated flush buttons. Six digital control inputs. Separate UV power button not listed in starting scope; LED meanings remain unresolved. The existing ESPHome device files and HA water-system package were read without changing live files or devices. That review stays on the local machine.

- 2026-09-17 simplicity decision (D39): Replaced hardware HOA bypass and failure-recovery exploration with input-only selectors, one controller/one supply, accepted repair downtime and breaker/disconnect seasonal shutdown. Updated active design, controls and parts documentation accordingly.

- 2026-09-17: Created project record from current equipment/cart description, directly retrieved September 14 discussion and targeted manufacturer checks. Identified coil-voltage mismatch, unverified contactor/supply/enclosure sizing, CT interface questions and UV-monitoring limits. Earlier suggestions remain labeled as proposals; no wiring or firmware generated.
- 2026-09-17 follow-up: Recorded confirmed 12V DC coil preference, user-reported equipment voltages/20A figures, Cactus identification and water-path order. Kept uncertain HP, flow units and cartridge descriptions unresolved. Added Cactus manufacturer pretreatment and anti-fouling review points.
- 2026-09-17 scope clarification: Closed tannin-filter relocation; retain existing position after UV. Adopted user-directed 20A breaker assumption and 3/4-1HP/240V pump planning range. Retained B03 DC-coil contactors as candidates; B04 AC-coil item requires replacement. Pump flow was repeated as 10 gph and is not a prerequisite to coil selection.
- 2026-09-17 contactor selection: Mike chose a third 25A/12V DC contactor instead of the 16A/12V AC unit. B03 quantity is now three; B04 is removed. Revised subtotal $267.76, or $266.36 with the pasted distribution-board coupon. No purchase made through this project.
- 2026-09-17 current monitoring/stock: Replaced active CircuitSetup/host plan with CT + conditioned ADC current detection at Mike's request. ADS1115 is a candidate, not a completed interface design. Recorded approximately two available 22mm buttons and one owned 12-to-24V converter. Recommended converter reuse if suitable; separate 24V DIN supply remains an option.
- 2026-09-17 operations/flow review: All five core operations are represented; triggers, timing and failure/restart policies remain to finalize. Assessed pictured DAE AS250U-100P and vertical alternatives, recorded 1-inch copper, and kept flow optional with no purchase added. See FLOW_OPTIONS.md.
- 2026-09-17 ultrasonic follow-up: Added clamp-on versus inline assessment and DAE U-100b as an investigation candidate; supersedes treating PD-100 as the leading overall flow candidate. No flow meter ordered or added to subtotal.
- 2026-09-17 value/sizing follow-up: Mike leans toward deferring the meter and is open to 3/4-inch devices. Use approximately 6 gpm for comparison. Recorded pressure-clogging versus media-capacity distinction and a specific meter pressure-drop example. No purchase added.
