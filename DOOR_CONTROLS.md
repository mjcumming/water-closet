# Local controls and indication

October 6 update: the installed door wiring is the bench map in [WIRING.md](WIRING.md) and [CONNECTIONS.md](CONNECTIONS.md). Tank selector is IN1 On and IN2 Auto. Pump selector is IN3 On and IN4 Auto. IN5 is the spin filter flush button. IN6 is the UV button. The four LED outputs are OUT9 pump, OUT10 tank, OUT11 spin filter flush, and OUT12 UV button. The three contactor outputs are OUT1 pump, OUT2 tank, and OUT3 UV. Flush valves are OUT4 and OUT5. The new local modes are Normal, Away and Shutdown. Mike dropped Maintenance in favor of the selectors, timed flush buttons and a web UV enable setting. Shutdown forces equipment off. The candidate's local selector start/stop, LED indications and UV timing are recorded in the current section of [DESIGN.md](DESIGN.md). `water-closet.yaml` was flashed over OTA on October 6; the initial API check confirmed Shutdown with both selectors Off and equipment commands off.

Updated October 6, 2026. Current design decision: simple controller-based operation, with two illuminated On / Off / Auto selectors and two illuminated flush buttons. Facing the knob, left is On, center is Off, and right is Auto. The checked terminals are the ones above.

Dashboard update: mode selection and equipment status are first, including a live UV start countdown. The web page offers Normal/Away selection only when both selectors are Auto; Shutdown is always available. **Advanced → Bench test** is collapsed during normal operation. It directly controls OUT1–16 while suspending automatic control and operating interlocks, and displays every input for wiring checks. Door selectors/buttons are observation-only during that session. All Outputs Off clears the outputs; Finish Test returns to Shutdown with everything off. Normal local selector behavior resumes after testing. Mike confirmed the connections using the dashboard tests; equipment and current sensors remain uninstalled. See DESIGN.md for the full test-session behavior.

## Everyday use and LEDs

Move both selectors to Auto to request Normal. Move both to Off to request Shutdown. Pump Auto with tank Off gives ordinary cold-water-only operation. Away is explicitly selected on the web page; unchanged Auto positions leave that choice in effect. A deliberate movement back to On or Auto requests Normal. Initial contact readings after boot do not count as movement.

| Light | Meaning |
|---|---|
| Pump selector | Steady: pump enabled; off: disabled; fast blink: invalid selector contacts |
| Tank selector | Steady: tank enabled; slow blink: heat requested but pump disabled; fast blink: invalid contacts |
| Spin button | Steady: automatic spin enabled for the current mode; fast blink: spin flushing; off when disabled or in Shutdown |
| UV/system button | Slow blink: waiting to start UV; steady: UV enabled; fast blink: system/Away flush; rapid flicker: calibrated UV low-current fault; otherwise off |
| Both button LEDs, two short flashes together then a pause | Wi-Fi disconnected; repeats every five seconds when no valve is active |

Slow means once per second, fast means twice per second, and rapid UV fault flicker means five times per second. Active flushing takes priority over the shared network pattern; a confirmed UV current fault takes highest priority on its LED. Current monitoring is initially disabled and uncalibrated, so absent CTs do not cause a fault. Local operation continues without Wi-Fi or Home Assistant. HA connectivity is shown separately on the dashboard. Intentional tank Off, Away Auto or Shutdown does not produce a blocked-heating blink. In normal operation, pump disabled always forces tank disabled, including tank On.

Away now has separate spin and system-water-exchange schedules, each with enable, one-to-four runs per day and duration. The top Away plan displays both settings and their next run/status. Spin runs first if both are due, followed by system flushing; tank and UV stay off. The spin button can start/cancel an Away spin run. The Run Away Flush web control starts a system exchange. UV lamp policy is unchanged. Equipment activity shows optional CT-based runtime/start estimates; calibration and UV fault setup are tucked under Advanced. See DESIGN.md for timer rules and monitoring limits.

## Settled design and rationale

Mike explicitly chose simplicity for a cabin water system used approximately 60 days per year. Use one 12V control power supply and one A16. A controller or supply failure is an accepted repair event; continued operation after either fails is not a design requirement. No numerical failure probability has been established, and limited seasonal use is not evidence of a particular reliability percentage.

Do not reopen dual supplies, an ORing/redundancy module, an external backup-supply transfer connection, mechanical contactor overrides, or hardwired selector bypasses merely because a hypothetical failure can be described. Revisit only if Mike requests it or actual operating experience establishes a concrete need. Ordinary electrical protection, correct component ratings and existing equipment protections remain part of the build.

## Local control architecture

All sequencing, permissives, UV timing, flush scheduling/timeouts and fault logic run locally on the A16. Home Assistant displays information and sends settings or requests; it does not referee individual devices. Retire or redirect existing HA enforcement routines during migration. Automatic operation and door controls must work without HA or network access, but depend on the A16 and its supply.

Pump and heater selectors are maintained On / Off / Auto (Hand or Manual, if used on a label, means a request to the controller). They switch ground to A16 digital inputs; they do not carry coil power. The A16 outputs operate the contactor coils through the final rated/suppressed output circuit. Existing pump pressure/motor protection and heater thermostat/high-limit remain in service.

Facing the knob, left is On, center is Off, and right is Auto. On is the Hand position. This is the order both selectors use.

| Selector position | On input contact | Auto input contact | Controller behavior |
|---|---|---|---|
| On, left | Closed to A16 ground | Open | Request equipment enable, subject to defined local operating interlocks and OEM controls |
| Off, center | Open | Open | Disable this output and reject automatic/remote enable while Off |
| Auto, right | Open | Closed to A16 ground | Follow local automation |
| Invalid | Closed | Closed | Inhibit output after allowing for normal switch transition/debounce |

Center Off is a software command, not electrical isolation or protection against a failed-on output. On is not a bypass around a failed A16. Input polarity is normalized in software; an open contact is not an externally driven zero-volt signal. No coil-voltage sensing is needed to determine selector position.

## Existing selectors are usable for the simpler function

The exact red DMWD listing inspected is B0DCHBC5Z1: https://www.amazon.com/dp/B0DCHBC5Z1 . Its eight pins comprise two C/NO/NC sections and LED terminals. The published contact diagram has section 1 C-NO at left, both sections C-NC at center, and section 2 C-NO at right.

For position sensing only, connect each section's common to A16 ground and its NO terminal to its own digital input. Leave NC terminals unused. This provides On / Off / Auto sensing using two inputs, without additional contacts or a sensing interface. Verify the actual terminals by continuity before wiring, and verify the blue variant has the same action. Keep LED supply wiring separate from the dry-contact inputs. Final A16 revision/terminal identification is still required.

The earlier DMWD limitation concerned doing hardware coil switching AND isolated position reporting simultaneously. That requirement is superseded. The more expensive IDEC four-NO switch and AutomationDirect modular alternatives are no longer needed to solve that problem; neither was selected or ordered.

## Other door controls and I/O budget

- UV power: the UV/system button requests flushing, not lamp-power toggling. UV follows the local mode/pump policy and web permission, with minimum-off timing described below.
- Flushes: one button for spin and one for UV/system. One valve at a time; the same button cancels its active run, the other button is ignored while busy. No request queue or extension of the current run.
- No local data display is planned. Use LEDs for local status and the web/HA interface for detailed readings.

| Input function | Starting build |
|---|---:|
| Pump On + Auto | 2 |
| Heater On + Auto | 2 |
| Spin-flush button | 1 |
| UV/system-flush button | 1 |
| Total / sixteen available | 6 |

Four door controls have 12V DC LEDs on OUT9–OUT12: pump selector, hot water tank, spin filter flush, and UV button. Three contactor coils are OUT1 pump, OUT2 tank, and OUT3 UV. Flush valves are OUT4 and OUT5. Output bank 2 uses the same 12V distribution as the board. Each LED negative goes to DC 0V. CT/pressure interfaces are separate from the digital switch inputs. Original A16 hardware reference: https://www.kincony.com/esp32-board-16-channel-relay-hardware.html .

## Seasonal shutdown and repair

Use the existing appropriate breakers/disconnects for positive seasonal shutdown of the water equipment; include the UV/control feed as applicable in the final operator procedure. Do not rely on door-selector Off or software Winter mode to prevent remote operation of winterized equipment. Keep any independently required room-heating circuit accounted for separately. Label the actual disconnects once the feeds are mapped. Deliberate return to service includes restoring water and completing the existing winterization/recommissioning procedure before enabling the heater.

If the A16 or supply fails, repair or replace it. An inexpensive service spare is optional, not a second installed supply or mandatory purchase. Document connections and normal settings so replacement is understandable. No automatic failover or controller-independent manual-operation test is required.

## Local rules and indication

- Door Off takes priority over remote/automatic requests in normal firmware operation. Final On-mode interlocks must be explicit; On does not bypass OEM protections.
- Pump disabled inhibits/cancels automatic flushing locally. Zero motor current alone does not mean disabled: the pressure switch can stop an enabled pump normally.
- Flushes have a bounded timeout that repeated requests cannot extend indefinitely. Define concurrent-request handling, and do not replay an interrupted flush after boot.
- Mode changes cancel obsolete work; an old Away flush must not later switch off a pump returned to Normal.
- Distinguish equipment ENABLED from actual RUNNING/HEATING. Current does not establish UV treatment dose; valve-command indication does not prove flow. Invalid sensor readings are not valid zero readings.
- The UV/system button LED reports pending UV start, enabled UV and active system flushing, with the shared Wi-Fi pattern taking priority only while both valves are idle. See the table above.

## UV timing and remaining work

The accepted A16 policy is a five-minute minimum-off period after boot or UV turn-off, with immediate turn-off when the current request is no longer permitted. A canceled request is never replayed. The UV button slowly blinks during a pending start and the dashboard displays the actual remaining time. This replaces the historical HA 60-second mode delay and immediate Sonoff toggle; it is a local control choice, not a manufacturer switching specification. Future current-based lamp-fault monitoring must start its grace period at each lamp energization, not controller boot.

Cactus guidance discourages frequent cycling and favors continuous operation during use. Its five-minute removal-cooling instruction does not establish a switching lockout: https://uvpure.com/Docs/Manuals/Cactus%20English%20Manual-Z800011G2.pdf .

Next: observe the implemented LED patterns on the door, finish equipment/sensor installation and calibration, and measure Away flush flow. Verify normal control, network-independent operation, restart defaults and bounded flushing during commissioning. These routine checks do not reopen the rejected redundancy architecture.
