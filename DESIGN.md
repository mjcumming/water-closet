# Design and operation

## October 6 control implementation — current working design

This section supersedes older mode names and flush-frequency proposals below. Mike has started the consolidated A16 firmware work. The water source is lake water. The A16 is to own equipment arbitration, schedules, flush sequences and timeouts; Home Assistant integration is a later step, and no Pi configuration is being changed.

The first candidate is `water-closet.yaml`, with hardware bindings in `firmware/board.yaml`, settings in `firmware/settings.yaml`, monitoring in `firmware/monitoring.yaml`, and the testable controller in `water_closet_control.h`. Existing `kc868-a16*.yaml` bench tests remain available. This is a development candidate, not a commissioned configuration.

### Modes confirmed by Mike

| Mode | Intended purpose |
|---|---|
| Normal | Ordinary cabin operation, including short UV/system refreshes and daily spin flushing |
| Away | Temporarily enable the pump and exchange water through the filter path while the cabin is unoccupied |
| Shutdown | All five equipment commands off; cancel any active/pending flush or UV start |

The final choice is three modes. Mike elected to drop Maintenance because the selectors, timed flush buttons and a web UV enable setting cover the useful manual controls. There is no Winter mode in the new controller. The old HA package's combined `Shutdown/Maintenance` behavior is historical reference only. Physical seasonal isolation remains the existing breaker/disconnect procedure.

### Current operating behavior

- Normal: selector On requests enable; Auto enables pump and tank; Off or invalid contacts inhibit that load. Tank and UV depend on the pump being enabled. Pump current is not the enable condition: the existing pressure switch may legitimately stop the motor.
- Away: Auto loads rest off between local scheduled exchanges. A door On request can enable its load, with tank still dependent on pump enable. An Away flush temporarily owns the Auto pump, keeps tank and UV off, and releases that ownership at completion. A mode change cancels the old run and computes outputs from the new state; there is no delayed pump-off action.
- Shutdown: all equipment off even with a selector at On. The web page, monitoring and mode selection remain available.
- Explicit pump/heater requirement: Mike confirmed that pump disabled must force tank disabled, even with the tank selector at On. This is already enforced in all modes; current measurement is not used as the pump-enable signal.
- Proposed button behavior: one valve at a time; pressing the same valve button cancels; another valve's button is ignored while busy. Runs are not queued or extended by repeated requests. Short spin/system-button runs are capped at 300 seconds; Away exchanges have a separate 1,800-second cap to accommodate a calibrated longer exchange. A one-second gap separates runs. These caps are candidate engineering limits, not measured flush requirements.
- UV timing: run continuously when requested; stop immediately when no longer requested; wait at least five minutes after boot or each turn-off before enabling again. The persistent web UV enable switch must also be on; switching it off cancels a pending start and commands UV off. Turning it on permits the normal mode/pump policy and does not override Shutdown. Only the current request is considered. Mike approved this minimum-off policy and the pending-start feedback October 6. It is not copied from the old 60-second HA delay or claimed as a manufacturer requirement.
- First boot defaults to Shutdown. Later boots restore the saved mode and duration/interval settings, then evaluate the actual selectors after input initialization. Active runs never restore. A restored Normal mode can therefore enable loads after startup; startup policy remains part of the operating review.
- Door LEDs follow the table below. They indicate commands and Wi-Fi association, not measured current, actual flow or end-to-end remote reachability. Bench testing retains direct ownership of all four LEDs.

### Selector positions

Facing the knob, both the pump and tank selectors use the usual Hand-Off-Auto order. **Left is On**, **center is Off**, and **right is Auto**. On is the Hand position. Mike adopted this October 6.

The bench found the tank's left end on IN1 and its right end on IN2. The pump's left end is IN3 and its right end is IN4. Center opens both contacts. So IN1 and IN3 are On, and IN2 and IN4 are Auto.

### Simple local start and stop

Mike's priority is reaching Normal and off locally. Do not infer Away from pump Auto + tank Off: that is a useful cold-water-only setup.

The candidate now handles local operation as follows:

| Deliberate selector action | Mode request / result |
|---|---|
| Both selectors Off | Shutdown; all equipment off |
| Move both selectors to Auto | Normal; enable normal automatic operation and web mode selection |
| Either selector moved to On or Auto | Normal; still honor the other selector and pump/heater interlock |
| Pump Auto + tank Off | Ordinary cold-water-only Normal operation; does not select Away |
| Pump Off + tank On or Auto in Normal | Pump and tank off; tank LED blinks because heating is blocked |
| Both selectors remain Auto | Follow the selected mode; unchanged positions do not cancel Away |

Away remains an explicit web/API selection. There is no two-button gesture, extra mode selector or hidden long-press action. A person can return from Away/Shutdown by deliberately moving a selector; cycling the pump Off then Auto is a clear local return to Normal. Merely publishing the initial selector readings at boot does not count as movement, so a saved Away/Shutdown remains in effect after restart. Both physically Off always requests Shutdown. On a fresh first boot the stored/default mode is Shutdown; use a selector movement to enter Normal.

Use Wi-Fi connection for the shared offline pattern rather than requiring an HA API client, since the controller must work autonomously. Detailed status, including blocked heating and UV pending time, remains on the web page. No separate LED code is added for every diagnostic or mode.

### Door feedback approved October 6

| LED | Normal indication | Attention / activity indication |
|---|---|---|
| Pump selector, OUT9 | Steady when pump command enabled; otherwise off | Fast blink for invalid selector contacts |
| Tank selector, OUT10 | Steady when tank command enabled; otherwise off | Slow blink when heat is requested but pump is disabled; fast blink for invalid selector contacts |
| Spin button, OUT11 | Steady when the automatic spin schedule for the current mode is enabled; off when disabled or in Shutdown | Fast blink while the spin valve is commanded open |
| UV/system button, OUT12 | Steady when UV command enabled; off when disabled | Slow blink while waiting for UV minimum-off timer; fast blink during system or Away flushing; rapid flicker for a confirmed UV low-current fault |
| Both button LEDs together | Usual individual meanings while Wi-Fi connected | Two short flashes then a pause when Wi-Fi is disconnected and neither valve is open |

Slow blink is 1 Hz (500 ms on/off); fast blink is 2 Hz (250 ms on/off); rapid UV-fault flicker is 5 Hz (100 ms on/off). The shared Wi-Fi pattern is 200 ms on, 200 ms off, 200 ms on, then off until the five-second cycle repeats. The shared pattern overrides idle status. Either active valve suppresses that overlay, preserving the individual flush/UV meanings. A confirmed UV electrical fault has highest priority on OUT12, including during flushing or Wi-Fi loss. Pump/tank indications are unaffected by network loss. Intentionally disabled heating in Away Auto or Shutdown does not cause a blocked-request blink. Normal timed flush buttons retain start/cancel behavior; they never toggle UV power.

The dashboard shows **Waiting to start** with a minutes:seconds countdown sourced from the controller's actual UV minimum-off timer. Cancellation removes the countdown; an elapsed timer does not start the lamp unless its current request is still permitted. Reloading the page does not reset that timer. There is no simulated lamp-health indication while CTs are absent.

### Flush settings and water-exchange rationale

Mike rejected the old four-hour spin schedule: once daily is sufficient as the starting frequency. The candidate's Normal-mode interval is now 1,440 minutes. The existing 30-second duration remains an editable placeholder pending review.

Keep the two system-valve purposes distinct:

1. Normal UV refresh: short flush, starting with the previous three seconds every 120 minutes. These are working settings, not measured cooling performance.
2. Away exchange: a longer run intended to refresh lake water in the four Big Blue housings, avoiding the stagnant-water condition Mike reports after absence. Mike confirmed an adjustable one to four runs per day with a separate runtime. The candidate defaults to two runs per day (12 hours apart) and a provisional 300-second duration; scheduled Away flushing defaults disabled until the run time is calibrated. The Run Away Flush button can exercise the sequence in Away mode.

Away scheduling is local elapsed time, not an HA or network-clock schedule. Mike clarified that the Away UV schedule means water flushing, with the lamp policy unchanged. There are two independent schedules: system/UV water exchange and spin flushing. Each has its own enable, one-to-four runs per day, and duration. System starts from two/day for 300 seconds; spin starts from one/day for 30 seconds. Both default disabled; an existing saved system enable is retained. Spin has a 300-second absolute valve limit; system exchange has a 1,800-second limit.

The first run is one full interval after enabling, changing frequency, entering Away, or rebooting in Away. Frequency options map to 24, 12, 8 or 6 hours between starts; missed runs are not replayed after reboot. Runtime edits apply only to future runs and do not move the schedule or extend an active valve deadline. A manual Away spin or system exchange restarts its respective interval. Disabling a schedule prevents future automatic starts; use Cancel to stop a current run. When both schedules are due, spin runs first, then system after the one-second valve gap. Each Away run temporarily owns the Auto pump, allows the one-second pump settling period, and keeps tank/UV off. Pump Off, unavailable control inputs, Shutdown or leaving Away cancels the run. The spin button can start/cancel a bounded Away spin run even with its schedule disabled. The existing Run Away Flush control starts a system exchange.

The top-of-page Away plan shows both configured frequencies/durations plus Disabled, Paused, Starting pump, Flushing now, Due/waiting, or the next run estimate. Remaining seconds come from the same timers that start the valves. Clock estimates use the browser clock and America/Chicago timezone and are marked approximate; neither internet time nor the browser drives the schedule. When control inputs/pump permission are missing, display the reason instead of promising a start time. Unchanged Auto positions preserve Away.

Use **gallons per flush** for exchange quantity and **hours between flushes** for frequency. Convert the chosen quantity to time using measured flow at the actual drain:

`run seconds = 60 × target gallons ÷ measured gallons per minute`

Mike requested a rough geometric estimate: four cylinders, each 4.5 inches in diameter and 20 inches long, total 5.508 US gallons. Adding 10–20% gives 6.06–6.61 gallons. Two to three nominal exchanges therefore span approximately 12–20 gallons. The proposed initial target is **15 gallons per Away run**. At a measured 2 gpm that would take 450 seconds; at 3 gpm, 300 seconds; at 5 gpm, 180 seconds. Those flow rates are examples, not measurements. No flow meter is planned, so firmware can time a run but cannot verify delivered gallons or compensate automatically for changing filter restriction. A timed bucket test across normal pump pressure cycling can establish the initial drain-flow estimate.

This is an intentionally approximate cylinder calculation using Mike's dimensions, not a measured installed water volume. The 4.5 × 20-inch designation normally describes cartridges. [Pentair's Big Blue specification](https://www.pentair.com/content/dam/extranet/web/nam/pentek/spec-sheets/310053-pentek-big-blue-spec-sheet.pdf) identifies 4.5-inch cartridges and lists housing dimensions, but does not specify installed water hold-up. The 10–20% allowance is for nearby plumbing; a long pump supply pipe and actual UV chamber volume may require a larger allowance. Refine the target during commissioning as useful. The system-flush takeoff must be downstream of all four housings to exchange water through all four; its location remains unconfirmed. Refreshing water is the control objective, not proof that an established buildup has been removed.

### Hardware, monitoring and validation status

Post-flash bench status, confirmed by Mike October 6: equipment/sensor installation is not complete and the CTs are not installed. Zero/unavailable pressure and absent current readings are expected now; do not treat them as commissioning failures or start troubleshooting them at this stage. CT sampling remains off. The earlier pressure-loop and ADS jumper checks remain valid historical bench results.

After using the dashboard wiring tests, Mike confirmed that the connections are correct. This closes the door/channel mapping check; it does not establish operation of uninstalled equipment or calibrated sensors.

UV switching, minimum-off timing and timed system flushing do not depend on the missing measurements. The A16 now has dormant current-based UV monitoring and usage counters described below. They require installed CTs and measured calibration before use; the Sonoff's watt thresholds are not copied into amp readings. Missing or stale samples remain unknown, rather than being interpreted as a failed lamp or a valid zero-current measurement.

- The October 6 bench wiring is in WIRING.md and CONNECTIONS.md: tank IN1/2, pump IN3/4, spin button IN5, UV button IN6. Four LED outputs are OUT9 pump, OUT10 tank, OUT11 spin filter flush, and OUT12 UV button. Three contactor outputs are OUT1 pump, OUT2 tank, and OUT3 UV. Flush valves are OUT4 and OUT5. Facing the knob, left is On and right is Auto.
- The flashed candidate commands those same contactor, valve, and LED terminals. Initial post-reboot checks are recorded below; physical commissioning remains to do.
- Pressure uses the confirmed onboard approximately 150-ohm receiver and the existing 4–20 mA conversion. Open-loop/out-of-range or stale data becomes unavailable, with raw voltage/current retained for diagnosis. Known-pressure calibration remains outstanding. Pressure is monitoring only until fault thresholds/actions are agreed; it does not prove the heater is full.
- One ADS1115 at `0x48` samples pump/tank/UV CT channels sequentially. Each acquisition must publish before the next starts; background ADC polls are disabled. CT sampling defaults off while the bias circuits are unfinished. Raw channels expose **uncalibrated AC signal volts**. New measured-current entities stay unavailable until positive calibration factors are entered. Calibration and measured run/off thresholds remain required; there are no current-based equipment trips or UV-treatment-health percentages.
- Wi-Fi and API connection loss do not trigger their default reboot timeouts. Local mode/selector decisions and flush deadlines continue without HA.
- Raw equipment switches are internal. All web flush controls go through the same local controller. Every output starts off, including unused output channels; a button held through boot is not treated as a fresh press.
- `tests/control_test.cpp` executes the actual controller code on the PC: selector/mode combinations, flush bounds/cancellation, scheduled Away operation, mode transitions, UV minimum-off timing, reboot and `millis()` rollover. Firmware validation and hardware commissioning are distinct checks.

October 6 validation result: `tests/run.ps1` passes, including the local selector-mode helper, blocked-heater indication, explicit pump-Off/tank-On combinations and 50,000 mixed input events. `esphome compile water-closet.yaml` succeeds with installed ESPHome 2026.7.2 / ESP-IDF 5.5.5. Image size is 900,583 bytes, with 49.1% application flash and 28.5% static DRAM used. Factory and OTA binaries are generated under `.esphome/build/water-closet/build/`. On Mike's instruction, the 09:47:20 CDT build was uploaded successfully over OTA. The encrypted API confirmed the new firmware after reboot: Shutdown, both selectors Off, all five equipment commands off, Wi-Fi connected, and no selector or input-communication fault. Pressure reported invalid/unavailable at this check; the earlier live-zero bench result remains historical. No Pi file was changed. LED timing, physical I/O, network-loss operation and real sensor performance still require the bench check.

The candidate's input/output assignments now match the clarified CONNECTIONS.md schedule: OUT1–5 drive equipment, and OUT9–12 drive only LEDs. Before installation, finish the operating choices above, finish/calibrate the CT interface and verify the candidate on the low-voltage bench. HA entity migration and retirement of old enforcement routines remain later work.

### Electrical monitoring and useful activity statistics

Mike requested code and design support for a failed/low-current UV lamp, pump runtime/cycles and similar heater statistics before installing the CTs. The independent logic is in `water_closet_monitoring.h`, with configuration and sensor entities in `firmware/health.yaml`. **CT sampling and UV current monitoring stay disabled for the current unwired bench.** Calibration and current thresholds default to zero, meaning unconfigured. Settings live under Advanced → Current sensors & UV fault setup.

October 6 deployment: the **11:15:02 CDT** build compiled/flashed successfully (916,331-byte application, 49.9% flash; 58,080-byte static DRAM, 32.1%). API verification found Away restored with both selectors Auto, both schedules disabled, CT sampling and UV monitoring disabled, all equipment commands off and no selector/input/output/UV-current fault. Pump/tank activity was Unknown with no accumulated runtime/starts. The live page enabled each schedule long enough to verify a 12-hour system and 24-hour spin next-run estimate, without energizing equipment, then restored both to disabled. HA connection indication was present after the named diagnostic client disconnected. Tests cover independent/competing due runs, cancellation, rescheduling, limits, rollover, UV fault grace/stale/recovery cases and usage estimates. The served dashboard bytes match the final source. Physical CT calibration and actual fault/LED observations remain pending.

- Each channel uses measured amperes per raw CT signal volt. Values are unavailable when sampling is disabled, calibration is nonpositive, the raw reading is invalid, or the last acquisition is older than 15 seconds. A fresh measured zero remains a legitimate zero only after calibration. CT sampling still defaults off after every restart.
- UV monitoring requires explicit enable and a positive minimum-current threshold, with a valid calibrated sample. It checks only while UV is commanded on and outside bench testing. Each command turn-on starts a new warmup grace (editable, initially 60 seconds); low current then has to persist for an additional delay (editable, initially 30 seconds). These are starting control settings to validate against the actual ballast/lamp, not manufacturer specifications.
- A confirmed low-current fault rapidly flickers OUT12 at 5 Hz and shows an explicit dashboard fault. It does **not** switch off the lamp, pump or heater. Normal current clears it; command Off or disabling monitoring clears it. Missing/stale samples clear the low-current confirmation and report unknown/waiting, not a dead lamp. A fresh fault must be established again after missing samples. Current can reveal an electrical problem involving the bulb, ballast, wiring or sensing; it cannot prove UV intensity, treatment dose, clean sleeves, safe water or actual watts.
- Pump and tank each expose calibrated current, Running/Idle/Unknown, estimated observed runtime and detected off-to-on starts since controller restart. Thresholds must be set from measurements. A 20% release hysteresis reduces threshold chatter. Initial/recovered readings that are already running do not invent a start; missing samples add no runtime. Long processing gaps are excluded. Counters do not persist through reboot.
- Existing CT acquisitions are 300 ms per channel, sequentially, once per ten seconds. Counters estimate operation between valid samples; short pump cycles can be missed and run times have sampling uncertainty. This is informational monitoring, not motor protection or an accurate energy meter. Faster acquisition, daily histories, cycle-duration summaries and persistent totals can be added after the installed signals and controller load are checked.

Keep fault reporting small and distinguish warnings from normal restrictions:

| Condition | Current behavior |
|---|---|
| Invalid selector contacts | Inhibit that load; fast blink its selector LED; page identifies invalid contacts |
| Control input communication fault | Inhibit normal equipment requests; page reports the fault |
| Output communication fault | Page reports output hardware trouble; a software command is not proof the output changed |
| Pump-disabled / heat-requested interlock | Tank stays off; slow tank blink; display Blocked by pump |
| UV low current, calibrated monitoring enabled | Rapid UV LED and dashboard fault; no new equipment trip |
| Missing/unconfigured CTs or pressure at this stage | Unknown/unavailable measurements; no invented load fault |
| Wi-Fi loss | Shared button double flash while valves idle; local operation continues |
| Home Assistant absent | Separate neutral connection status on page; no door fault pattern or control interruption |

HA status identifies an authenticated native API client whose reported name is Home Assistant. Browser/log/diagnostic connections do not count. It reports the HA-to-A16 connection only, not automation health or internet reachability; it does not require polling the Pi or changing its configuration. Both Wi-Fi and API reboot timeouts remain disabled. Unexpected current while commanded off, pressure-loss response, excessive run length and short cycling are possible later diagnostics, but need installed-equipment baselines before thresholds or automatic actions are assigned.

### On-device dashboard and bench testing — October 6

Mike requested a usable local web interface with mode control at the top and direct input/output testing. The dashboard is bundled into the firmware through ESPHome's `js_include`/`css_include`; it uses the local REST API and event stream, with no CDN or HA dependency. See [ESPHome web server](https://esphome.io/components/web_server/) and [web API](https://esphome.io/web-api/).

The everyday dashboard now puts mode selection, both selector positions, uptime, pump/tank commands, UV status/countdown and flush status/countdown first. The connection badge reports browser-to-A16 connectivity. Manual flush controls and UV permission remain readily available, with an expandable door-LED guide. **Advanced → Bench test** is collapsed during normal operation; all sixteen input/output controls and the input activity log remain there. Entering an active test session opens Advanced and clearly labels the top mode as Bench test. Schedules and uninstalled sensor readings stay in separate collapsed sections.

Deployment verification: the October 6 **10:47:27 CDT** build compiled and flashed over OTA (907,395-byte image; 49.4% application flash; 55,712 bytes static DRAM, 30.8%). The encrypted API confirmed the running build and `UV start remaining` sensor at zero in Shutdown, with both selectors Off and no selector/input-communication fault. The live page verified Advanced collapsed by default, all sixteen input/output controls retained, bench session entry/exit and return to Shutdown with all output commands off. A local UI fixture checked the 4:32 pending-UV display and disconnection behavior; controller tests checked countdown cancellation/rollover, LED pulse boundaries and flush/network priority. Live pending-UV and the new door light patterns still need observation with the physical selectors operated; equipment/sensor commissioning remains outstanding.

- The Normal and Away buttons require both selectors in Auto and bench testing off. This restriction is enforced in firmware for web and native API requests, as well as in the page. Shutdown remains available regardless of selector position. Deliberate local selector movement still requests Normal as previously defined.
- Normal manual controls retain the timed spin/system flush requests, cancellation, Away exchange request and UV permission setting. These use the existing interlocks and UV timing policy.
- Bench testing is a separate, explicit commissioning session, not a fourth operating mode. Entering it cancels normal control, clears pending flushes and sets every output off. While active, OUT1–16 can be toggled independently, including unused channels and LEDs; selector positions, pump/heater interlock, valve timers and UV anti-cycling do not arbitrate these direct wiring tests. This exception is for the currently unwired bench assembly Mike described.
- All sixteen inputs remain visible. Door controls are observed without starting flushes or changing operating modes during testing. The page records input transitions for brief button tests; the log is browser-local.
- All Outputs Off clears the test output mask. Leaving bench testing or selecting Shutdown cancels the session, clears all outputs, and leaves the operating mode at Shutdown. Unchanged selector positions do not immediately restart equipment. Test mode and output states do not restore after reboot.
- Raw GPIO switches remain internal; public test switches accept output commands only while the firmware owns an active test session. The normal controller and bench path share one output writer so they cannot compete. Status entities show the actual GPIO command states during tests.
- Losing the browser connection disables its controls and shows Disconnected. Existing test output commands remain until changed, All Outputs Off, leaving test mode, Shutdown, or controller reboot; browser disconnection is not an automatic shutoff.
- The unused second input bank at `0x22` is now read for IN9–16. Missing pressure/current readings remain expected; this work does not add electrical-health alarms or infer flow.

Mode persistence now uses the guarded mode setting's own storage. The first upgrade to this dashboard starts in Shutdown; later boots restore the selected operating mode. Physical outputs still start off and no active test/flush is restored.

Initial dashboard verification: build `2026-10-06 10:28:10 -0500` compiled and flashed successfully (905,403-byte image, 49.3% application flash, 30.8% static DRAM). The browser verified bench entry, OUT9 LED on, All Outputs Off and Finish Test back to Shutdown. Live REST checks verified that a direct OUT9 request is ignored outside testing, Normal is rejected with both selectors Off, a test LED command survives automatic refresh, and exit clears the output. All sixteen input/output bindings were checked in software; Mike subsequently confirmed the physical connections. Operation of uninstalled loads/sensors remains to be commissioned. The installed ESPHome build emits legacy entity IDs in its event stream but routes REST commands by URL-encoded entity name; the dashboard uses the names received from the device for commands.

Updated: October 4, 2026. [CONNECTIONS.md](CONNECTIONS.md) owns the assigned channel/connection schedule. Mike accepts the ordered hardware, selected contactors and ratings and owns CT/ADS assembly wiring. Earlier procurement/rating questions are historical, not gates to assigning channels. Operational software thresholds and timing still need to be finalized.

## Equipment record

October 4 work boundary: allocate the three selected contactors to pump, tank and UV; both flush valves discharge outside. Do not reopen contactor ordering or load ratings. Preserve the older survey record below as history; the active connection assignments are in CONNECTIONS.md.

| ID | Equipment | Known purpose | Information still needed |
|---|---|---|---|
| L01 | Hot-water tank, 240V, assumed 20A breaker | Enable/disable tank heating | Model/watts; existing thermostat/high limit; draining procedure |
| L02 | Well pump, 240V, assumed 20A breaker, 3/4-1HP | Enable/disable water supply | Use 1HP for preliminary motor-duty selection; actual model/FLA and starting requirements; pressure switch/control box; pressure tank and dry-run protection |
| V01 | Spin-filter flush, one 12V solenoid valve | Purge sediment from spin filter | Exact ordered model, DC/coil data, pressure range, drain route, effective purge duration |
| V02 | System/UV flush, one 12V solenoid valve | Flush cold-water treatment path before the house; also refresh/cool UV chamber | Exact ordered model, DC/coil data, takeoff, drain capacity and actual flushed volume/coverage |
| L03 | Cactus Pure UV, 120V, exact model pending | UV treatment with electrical monitoring | Likely UV Pure Cactus family; nameplate/model, watts, ballast startup behavior, monitor model and alarm connections |
| C01 | Standard KinCony KC868-A16, selected October 4 | Main controller; listing image shows REV1.4 | Confirm physical PCB revision and matching schematic before final terminal-level wiring |

Mike confirmed the supply voltages and requires 12V DC contactor coils. On September 17 he directed us to assume 20A refers to breaker size, with a likely 3/4HP pump and 1HP as the upper planning assumption. These are sufficient to continue preliminary selection, not verified nameplate data. He repeated 10 gph for pump flow; retain that report without silently converting to gpm. Actual flow/UV capacity can be established when selecting sensors; it does not determine contactor coil voltage. Keep the two 25A contactors as candidates pending exact-model motor-duty verification. The third contactor's role remains unconfirmed.

Contactor selection update: Mike selected three matching 25A, 2-pole 2NO contactors with 12V DC coils, replacing the earlier 16A AC-coil candidate. This supersedes the original two-unit count; final load/interface checks remain unchanged.

## Architecture

### Selected controller hardware — October 4

Mike supplied a listing screenshot of the standard KC868-A16, with PCB marking REV1.4. This establishes the intended model and pictured revision, not the revision of the physical unit. The labeled connectors show sixteen dry-contact digital inputs, sixteen DC outputs in two banks, four analog channels, Ethernet, RS485 and an I2C header.

KinCony's hardware documentation identifies these outputs as MOSFET DC outputs for external relays/contactors/valves, rather than onboard mains-switching relay contacts. The screenshot separately labels the controller's 12V power input and each output bank's 12V/24V supply input. Account for both output-bank supply connections in the low-voltage wiring plan; powering the controller alone is not the complete load-power wiring. The single 12V supply architecture is retained, subject to the existing total-load check. Do not assign a per-channel or total-bank load rating from this screenshot.

The six selector/button inputs fit within sixteen digital inputs. Four load commands (pump contactor, heater contactor and two flush valves), four independently driven door LEDs, and an additional UV contactor command if retained would use nine outputs. LED drive compatibility and coil/valve current/suppression still depend on the actual components.

KinCony's later analog-input support describes A1/A2 as 4-20mA and A3/A4 as 0-5V, whereas its older hardware article describes all four as voltage inputs. Treat the physical board's input circuitry as controlling for the owned LEFOO 4-20mA sensor; confirm the pictured REV1.4 unit's actual revision/circuit and calibrate before assigning its pressure channel. The published schematic is dated February 3, 2025 and is not labeled as a verified match to the physical unit. Preserve the existing 24V pressure-loop test plan. Sources: [KinCony hardware details](https://www.kincony.com/esp32-board-16-channel-relay-hardware.html), [analog-input support](https://www.kincony.com/forum/showthread.php?tid=7890), [published schematic](https://www.kincony.com/download/KC868-A16-schematic.pdf).

D39 (September 17): one A16 and one 12V control supply. Door selectors report On / Off / Auto through ground-switched inputs; A16 outputs operate coils. On is a controller request, not a hardwired bypass. Controller/supply failure is an accepted repair event. No dual supplies, redundancy module, backup transfer connection or mechanical override is planned. Use breakers/disconnects for seasonal shutdown. Preserve OEM and ordinary electrical protections. Do not reopen redundancy for hypothetical failures; see DECISIONS.md D39 and DOOR_CONTROLS.md.

```text
Door controls + water sensors ----> Existing A16 / local ESPHome logic
                                         |
                               Verified output interfaces
                                         |
                            Contactors and two flush valves

Electrical monitoring -----------> A16 where local decisions need it
                                  Home Assistant for display/history

Existing equipment controls and protections remain in their control paths.
```

The goal is one enclosure, with a distinct mains section and low-voltage/sensor section, protected terminals, an accessible service disconnect arrangement, and room for wiring and maintenance. The current direction is to keep current measurement on the A16 through a suitable ADC interface, without a separate metering host.

### Current detection: initial-build scope confirmed October 4

Install three current sensors in the initial build: hot-water tank, UV light and pump, explicitly confirmed by Mike October 4. The interface baseline remains three conditioned CT channels into one owned HiLetgo ADS1115, read locally by the A16 over I2C. Current readings support actual electrical-operation indication alongside contactor enable commands; the two states remain distinct. Final circuit, sequential sampling and calibration require bench validation.

### Upper-compartment hardware overview

ADC carrier selected for the parts list: MECCANIXITY black DIN carrier for a 100 x 72mm PCB, shown at $9.69, one unit. Listing specifies 35mm rail and 1.5-2.0mm PCB thickness. Use a fitting interface/prototype board to support one owned ADS1115 plus three CT input-conditioning circuits and secure connectors. The small ADS1115 breakout does not directly span the carrier's board guides. Prototype-board inclusion is ambiguous in the listing; verify supply contents. Board envelope is approximately 3.94 x 2.83 inches; overall carrier dimensions and length occupied along the rail are not yet established. Component layout and upper-compartment fit remain to check; no wiring or board fabrication performed.

Latest simplification: target one owned ADS1115 using three of its four single-ended inputs, with one small DIN-mounted interface assembly. Three separate ADCs were a suggested way to avoid channel switching, not a requirement; they are not the current baseline. One ADC requires non-overlapping per-channel waveform sampling and validation in ESPHome; three overlapping CT sampling windows must not compete for its multiplexer. Retain spare owned modules as fallback only if the single-board implementation proves inadequate. No additional DIN terminal blocks are planned beyond Mike's two DC distribution blocks; mount required CT connectors/terminations on the interface assembly and use existing A16 terminals where suitable.

DC protection scope: do not assume a fuse per device or a DIN fuse bank. Determine whether the selected supply's documented overload/short-circuit behavior safely limits current for every downstream wiring/connector path and satisfies the equipment instructions. Its 2A nameplate alone is not a verified hard current limit. Additional branch protection is conditional on that assessment, converter characteristics and any desired fault separation, not an automatic shopping item. This is separate from the CT signal bias/input protection (needed to keep ADC pins within limits) and coil/valve transient suppression (where not already provided).

Latest low-voltage inventory/layout recap: Mike identifies the DIN rail, A16 and two low-voltage terminal blocks as the starting set. Provisionally assign those blocks to +12V distribution and DC return; exact topology/ratings remain to verify. Keep the pressure converter's +24V separately identified. Remaining physical items are the owned 12-to-24V converter, the mounted CT/ADC assembly, appropriate DC branch protection and coil/valve suppression where not already provided, plus labeled field/door wiring connections.

Proposed ADC mounting: use one compact DIN carrier or small enclosure holding a securely mounted interface board and one owned ADS1115. Include per-CT bias/protection and mechanically supported CT connectors or screw terminals on that assembly; use standoffs and a short I2C connection to the A16. Breakout boards are not DIN-mountable on their own. Select the circuit and connector arrangement before sizing the carrier; no footprint or complete upper-rail fit has been established. A small interface board is still required by this proposal and is not replaced by the bare ADS1115 module. No separate controller or general-purpose relay bank is planned.

The next layout step after the A16 is its current-measurement interface. Reserve one small mounted assembly beside the A16 for CT connections, per-channel signal bias/protection and the candidate ADC, keeping the I2C wiring short. Three measured loads and one spare CT remain the plan; ADS1115 quantity/sampling and UV sensitivity are not yet validated. No CircuitSetup or second controller is planned.

| Upper-compartment item | Current disposition |
|---|---|
| A16 in proper DIN carrier/housing | Owned main controller |
| CT signal-conditioning and ADC assembly | Needed for selected CT approach; exact assembly/components to select |
| Low-voltage distribution and branch protection | Prefer owned DIN distribution device if suitable; separate 12V positive and DC return connections, plus separately identified 24V if used |
| 12-to-24V converter | Owned reuse candidate, only needed if selected pressure sensors require it |
| Pressure input adaptation | Conditional on sensor output and actual A16 input range; do not buy generic converters before identifying sensors |
| Labeled low-voltage connection points | Pressure sensors, flush valves, door controls and optional leak input; use A16 terminals directly where suitable, add field terminals where service access benefits |
| Coil/valve output interface and suppression | Direct A16 drive where verified compatible; extra driver only if required by load/board ratings; no blanket extra relay bank |

Control power supply remains on the lower rail. Door selectors/buttons are door-mounted, external pressure sensors/valves/leak probe are outside the enclosure, and their connections occupy the upper compartment. Keep electrically noisy coil/converter wiring away from sensitive CT/ADC wiring. No new flow or room-temperature module is required by the present scope. Component/terminal count and upper-rail fit remain to establish after selecting the CT assembly and identifying the pressure sensors.

### CT interface details

Owned ADC identified: Mike supplied the HiLetgo ADS1115 16-bit four-input I2C breakout listing, sold as a three-pack. Reuse one module for the three initial-build CT channels; available physical quantity is not independently counted. Validate non-overlapping channel acquisition and the A16 I2C connection. Keep the fourth channel unused initially so unrelated reads do not interfere with CT acquisition. The earlier one-ADC-per-CT proposal is a fallback only if the single-module implementation proves inadequate; it is not the baseline or an automatic purchase trigger.

Each CT still needs a bias/protection interface; the pictured breakout is an ADC, not a complete CT input circuit. Prefer 3.3V operation compatible with ESP32 logic, subject to actual A16 header/pullup verification; never feed the module from 12V/24V. Define bias, gain and headroom before wiring; even differential mode does not permit input pins to swing below the supply rails. Verify UV sensitivity and calibrate all channels before replacing the existing UV monitor. The updated module choice does not make this an energy meter or a UV-intensity measurement.

Mike prefers the four-pack of split-core current transformers (CT clamps) with an analog-to-digital converter, rather than CircuitSetup. Use current/running detection as the working scope. CircuitSetup and its separate host/RS485 link are removed from the active plan; the earlier discussion remains historical. Current-only measurement does not independently provide real watts, power factor or kWh.

```text
CT around one load conductor
         |
AC signal bias / input protection (one channel per CT)
         |
External ADC, ADS1115 candidate
         |
Short I2C connection to A16 -> ESPHome current / running status
```

Proposed assignment: heater, pump, UV, and one spare CT. The pack is four sensors, not a complete four-channel measurement board. Its advertised output is 1V at 20A; that is an AC measurement signal, not a ready-made 0-1V DC reading. Voltage-output CTs normally contain their burden resistor; verify the actual purchased model and do not blindly add the burden used for current-output CTs. Bias and protection are still needed. [S08]

ESPHome documents CT measurement using an ADC source, including ADS1115, with calibration. Its ADS1115 component requests continuous mode for CT use. This supports the approach, not an untested wiring assembly. [S04, S07]

An ADS1115 offers four single-ended inputs sharing one converter. It is a candidate for periodic load monitoring, not simultaneous waveform capture: schedule sampling windows/channel switching and validate all active channels under ESPHome before finalizing the module count. October 6 confirmed the A16 I2C header, 3.3 V power, and address `0x48`. CT waveform sampling is still open. [S07, S09]

Interface design tasks:

- Keep the AC waveform within ADC pin limits using a suitable bias/protection circuit. A 1V RMS sinusoid has about 1.41V peak excursion, so range cannot be assessed using 1V alone. Include startup/transient headroom; do not connect raw negative swings to a ground-referenced ADC input.
- Do not assume the A16's existing DC analog input terminals accept raw CT waveforms. Their conditioning/sampling path needs separate analysis if used instead of an external ADC. [S01]
- Check the 20A CT range against normal heater/pump current and starting behavior. A 20A breaker is not a 20A ceiling on brief motor-start current. This design is for running detection and periodic RMS readings, not accurate inrush capture or motor protection.
- Validate the much smaller UV signal against noise and actual off/start/run readings. Keep the current working UV monitor as a reference and fallback until the new channel performs adequately.
- Calibrate with a reference instrument and use persistent thresholds/hysteresis for running alarms; separate enabled, running and unavailable states. A thermostat-controlled heater or pressure-controlled pump can correctly draw zero while enabled.

### Pressure-sensor power and available controls

Current decision: Mike declined the Ashcroft route on cost and will try the single owned LEFOO. Proceed with a 24V test at known pressure; no replacement or second pressure sensor purchase planned now. Ashcroft material below is historical research only, not an active selection task. Successful operation and winter survival have not been established.

Latest test/environment clarification: Mike has never tested the owned LEFOO at 24V. The 12V/A16 attempt used mouth-blown air, not a gauge-verified water pressure or direct loop-current measurement. Normal water operation is approximately 30-50 PSI. Record this as an inconclusive low-pressure readout test, not proof of a failed sensor or proof that 12V cannot work. A healthy powered 0-100 PSI/4-20mA transmitter should nominally produce 4mA at zero gauge pressure, 8.8mA at 30 PSI and 12mA at 50 PSI. No displayed change is distinct from verified zero loop current. A small mouth-generated pressure can be hard to resolve against the listing's +/-1 PSI full-scale accuracy; do not adopt the user's estimate of 1-2 atmospheres as measured applied pressure.

New selection requirement: cabin plumbing is blown out with compressed air for winter, but the water closet reaches freezing temperatures afterward. Lowest temperature is not yet specified. Require a defined winterization method that prevents retained water from damaging the sensor, or a manufacturer-documented option for water freezing in the sensing cavity. A subzero ambient/storage rating, waterproof housing or 4-20mA output does not establish this capability. Mike reports previous cheap voltage-output sensors failed quickly; cause remains unknown and voltage output itself is not the cause established by that history. Do not treat draining the main pipe as proof that a dead-ended sensor cavity is empty.

Recommendation: perform a proper 24V loop test of the existing LEFOO against a reference gauge at zero and approximately 30-50 PSI, with its actual pinout/input circuit verified, before discarding it or buying a second. Separately resolve winter survival. A removable sensor with an accessible isolation/service fitting is a practical alternative to a freeze-resistant model, but has not been accepted as the user's winter routine. Upright/drainable mounting or a flush diaphragm may reduce retained water but is not by itself a freeze guarantee.

Replacement research candidate: Ashcroft G2 or T2 with the explicitly ordered XUP freeze-proof option and suitable 4-20mA/range configuration. Ashcroft describes filling the diaphragm cavity with a stable emulsion that excludes water and transmits pressure; their note specifically addresses residual water after purge/draining. A standard G2/T2 without XUP is not equivalent. Verify exact option availability, cold limit, accuracy effects, connection, water-use suitability including potable requirements and price before selecting. No vendor contacted and no replacement selected. Sources: https://www.ashcroft.com/wp-content/uploads/2020/09/pi-page-freeze-proof-option-xup-pressure-transducers-tr-pi-23.pdf ; https://www.ashcroft.com/products/pressure/pressure-sensors/g2-pressure-transducer/ ; https://www.te.com/en/whitepapers/sensors/effects-of-cold-climates-on-pressure-transmitters.html

Mike identified exactly one owned LEFOO T2000 pressure transmitter from its purchase listing: 0-100 PSI, 4-20mA output, advertised 8-36VDC, G1/4 male pressure port, Hirschmann connector with 1m cable, +/-1% full-scale accuracy. This supersedes the unidentified 24V-only sensor assumption. Nominal scaling is 4mA = 0 PSI, 12mA = 50 PSI and 20mA = 100 PSI. Confirm the actual unit label and connector pinout before connection. Validate one against a reference pressure gauge before adding a second for simultaneous upstream/downstream filter differential. One installed sensor measures pressure at one location, not differential pressure.

Updated by actual test history: Mike reports an earlier attempt powering the transmitter with 12V from the A16 produced no signal/readout, and his earlier research indicated using 24V. Retain the owned 12-to-24V converter as the active pressure-supply plan; do not require another 12V experiment. A successful 24V test has not yet been reported. The failed attempt does not by itself distinguish insufficient loop voltage from connector/wiring/input/configuration problems. During commissioning verify the 24V-powered loop and A16 input separately, rather than promising that higher supply voltage alone fixes the issue.

Voltage-budget background: the listing advertises an 8V minimum, while current LFT2000 family documentation lists 10-36V for 4-20mA. A 150-ohm receiver drops 3V at 20mA, leaving only 9V from nominal 12V before wire losses, versus 21V from 24V. The newer family documentation does not establish the older owned T2000's exact requirements, but using 24V gives greater loop headroom within both stated ranges. No additional mains supply is planned. Manufacturer family reference: https://www.lefoo.com/products/general-type-pressure-transmitter-lft2000/

The A16's pressure interface may already support 4-20mA, depending on physical board revision/input population. KinCony documentation varies: https://www.kincony.com/forum/showthread.php?tid=7890 versus https://kincony.com/forum/showthread.php?tid=6539 . Verify the owned board before assigning a channel or adding an external resistor. Do not place an extra shunt across an existing current-input burden without accounting for the combined circuit. Prefer the A16 pressure inputs or an appropriate current receiver, keeping the owned ADS1115 modules dedicated to CT waveforms. Plumbing note: G1/4 is a parallel pipe thread and needs the matching adapter/sealing arrangement; do not force it into 1/4 NPT.

For scale, two ordinary two-wire 4-20mA loops at 24V and 20mA each draw 0.96W total from the 24V supply. At an illustrative 80% converter efficiency, that is 0.10A from 12V, plus converter idle demand. Budget fault/overrange demand and margin separately. This is a calculation for a possible sensor pair, not measured consumption of Mike's unidentified device.

Some nominal 24V pressure transmitters also operate at lower voltage, but the sensor minimum plus input/wire voltage drop must fit the supply at maximum loop current. Confirm the exact sensor before deciding whether the 24V conversion is necessary. WIKA's 4-20mA A-10 is an example with an 8-30V range; it is not an identification or recommendation of Mike's sensor. [S10]

Keep the CT ADC channels separate from pressure inputs where practical. Pressure signal type (4-20mA versus voltage), input burden and grounding must be matched; a 24V sensor supply does not make its signal a 24V ADC input.

Mike has approximately two leftover 22mm pushbuttons from the Ultra Lift project. Allocate them provisionally to spin flush and system flush; verify momentary action, contact blocks and any illumination voltage before assigning wiring. Do not order duplicate buttons now.

## Water path: user-reported September 17

```text
Well pump -> Spin filter -> Big Blue three-housing bank -> Cactus UV
                |                                            |
         Own flush valve                         Single Big Blue housing
                |                                     (tannin filter)
         Outside discharge                                  |
                                                        Cabin supply

Separate system-flush branch: used to flush/cool UV;
exact tee relative to final housing still to pin down; discharges outside.
Pressure tank / pressure switch location not yet recorded.
```

Mike confirmed both flush outlets discharge outside. Exact outlet routing and seasonal freeze exposure can be recorded during the build survey; the brief reply did not unambiguously locate the system-flush tee relative to the final housing.

Cartridge recollection was “15 to 20,” “20 to 5 carbon filter,” UV, and “10 filter.” These numbers may refer to micron ratings but units/types were not confirmed. Do not assign them one-to-one to the four housings yet; obtain labels for the three bank cartridges and the final cartridge.

**Settled project scope:** Mike reports that the existing arrangement works well and directs that the tannin filter remain after UV. Preserve this plumbing order; do not carry relocation as an open item or ordering prerequisite. The earlier manufacturer-placement observation is background only, not a requested plumbing change or a certification of treatment performance. [S06]

The manual also describes an anti-fouling drip system that moves water through UV during no-use periods. Determine whether Mike's flush replaces or supplements it; no equivalence or flush interval is established. [S06]

A single flush takeoff only moves water through the path leading to that takeoff; it cannot be assumed to flush every dead-end cabin branch or the water heater. Preserve the user's name “system flush,” but define its actual coverage.

### Sensors under consideration

| Measurement | Useful result | Placement/interface decision |
|---|---|---|
| Pulse flow meter | Volume, estimated flow rate, usage-based flushing | Prefer if gallon tracking matters; choose pipe size, pressure loss, potable-water suitability, pulse resolution and maximum frequency |
| Flow switch/detector | Water moving or stopped | Simpler if volume is unnecessary; cannot support gallons-based maintenance |
| Upstream pressure | Available source/system pressure | Locate relative to pressure tank and filters; choose range and signal |
| Downstream pressure | Delivered pressure; filter differential when compared upstream | Measure comparable points at a known flow; static pressure alone is weak evidence of filter loading |
| Pump/heater/UV electrical readings | Actual load operation, runtime and abnormalities | Commanded state and measured activity are separate fields; an enabled heater/pump can normally idle under its own controls |
| UV chamber/pipe temperature | Excluded from initial build | Mike prefers periodic flushing without added temperature-sensor installation |
| Optional floor leak input | Local water alarm | Alarm first; define isolation action separately |

Meter position determines what it counts: cabin use, system-flush waste, and spin-filter waste may cross different paths. A downstream meter will not automatically count an upstream purge. Pick its location deliberately.

Latest sizing basis: approximately 6 gpm, estimated by Mike; 3/4-inch meters are eligible if added pressure loss is acceptable. This supersedes older flow-unit recollections for preliminary comparison, not verified pump data. Mike leans toward deferring flow measurement. Pressure difference under repeatable flowing conditions detects restriction, not carbon/tannin removal capacity. Across-bank sensing does not identify an individual clogged cartridge. See FLOW_OPTIONS.md.

## Logical inputs and outputs

Installed wiring, checked October 6: tank IN1 On (left) and IN2 Auto (right); pump IN3 On (left) and IN4 Auto (right); IN5 spin filter flush button; IN6 UV button. LED outputs OUT9 pump, OUT10 tank, OUT11 spin filter flush, OUT12 UV button. Contactor outputs OUT1 pump, OUT2 tank, OUT3 UV. Flush valves OUT4 and OUT5. A16 A1/GPIO36 is pressure. ADS A0/A1/A2 are reserved for pump/tank/UV current. Facing the knob, left is On and right is Auto. CONNECTIONS.md and WIRING.md own the terminal record.

This is a function inventory, not a GPIO or terminal map. Verify A16 output type, per-channel/group limits, inductive-load protection and input polarity against the actual board. Check pulse capture performance before choosing a high-frequency flow sensor. [S01, S02]

| Direction | Logical function | Status |
|---|---|---|
| Output | Heater contactor command | Core |
| Output | Pump contactor command | Core |
| Output | Spin-filter valve command | Core |
| Output | System-flush valve command | Core |
| Output | UV power-switch command | Core; switching device and normal operating policy unresolved |
| Output | Common alarm/door indication | Proposed; count depends on desired indicators |
| Input | Pump selector state | Two ground-switched inputs: On and Auto; software interprets neither as Off |
| Input | Heater selector state | Two ground-switched inputs: On and Auto; software interprets neither as Off |
| Input | Separate UV power button | Earlier proposal; not listed in October 4 starting scope |
| Input | Spin-flush control | One illuminated pushbutton input. Controller request only. |
| Input | UV/system-flush control | One illuminated pushbutton input. Controller request only; not UV power toggling. |
| Input | Flow pulse or flow status | Excluded from initial build |
| Analog or digital bus | One pressure sensor | Initial build; exact A16 channel/interface to verify |
| Input | UV manufacturer's fault output | If available and electrically compatible |
| Input | Leak/other fault signals | Optional |
| Meter data | Heater, pump, UV measured current/power | Existing UV plus proposed additional monitoring |

October 4 input budget: six of sixteen for pump/heater selectors and two flush buttons. Four illuminated door controls; reserve four indication outputs if independently driven. One 12V valve per flush is confirmed. Pump, heater and two valves use four load commands; retaining A16 UV power control would add a fifth, subject to final UV policy. CT/pressure interfaces are separate. See DOOR_CONTROLS.md for position decoding and accepted controller dependence.

All water-system arbitration runs locally on A16; HA supplies requests/settings and displays history. Retire/redirect existing HA per-device enforcement during migration. Door Off inhibits remote/automatic requests in normal firmware operation. Positive seasonal shutdown uses existing breakers/disconnects; selectors are not isolation. Both On and Auto require a working A16 and DC supply (D39).

## Operation coverage review — September 17

The functional inventory covers all five requested operations. This is functional coverage, not a completed wiring/firmware design.

| Operation | Planned control | What still needs definition |
|---|---|---|
| Pump enable | Contactor, HOA selector, current feedback | Final local/automatic enable policy and fault recovery |
| Heater enable | Contactor, HOA selector, current feedback | Normal schedule/enable policy and service/winterization behavior |
| UV power | Third contactor provisionally assigned here; current feedback | Confirm allocation, normal continuous-on policy, local service control and restoration after outage |
| Spin-filter purge | Valve, manual button, bounded automatic routine | Trigger and duration; timed operation does not require a flow meter |
| System flush / UV refresh | Valve, manual button, bounded automatic routine | Refresh trigger and duration, relationship to current anti-fouling hardware, takeoff location |

Across these operations, finish manual-versus-auto priority, valve maximum-on time, handling concurrent requests, power-loss/restart defaults, invalid-sensor behavior and alarm/reset behavior. Monitoring remains current plus proposed pressure inputs, with flow optional. No extra core actuator was identified as missing; an automatic main water shutoff is a separate optional feature.

Mike confirmed the prospective meter location uses 1-inch copper and prefers vertical installation. See [flow assessment](FLOW_OPTIONS.md): the pictured AS250U-100P is horizontal and reports only every 10 gallons. A meter is useful for volume/abnormal-use monitoring but is not required to complete core controls. Retain a flow-input provision; no meter purchase selected.

## Proposed operating behavior

The controller-based On / Off / Auto architecture and seasonal disconnect approach are settled by D39. Detailed thresholds, timings and restoration policies remain proposals.

| Situation | Proposed behavior | Unresolved detail |
|---|---|---|
| Normal operation | A16 makes local decisions; Home Assistant displays settings/history | Which functions are automated immediately |
| Pump/heater On (Hand) | A16 reads selector and requests equipment enable; OEM protections remain | Final local interlock priority; no controller-independent bypass |
| Pump/heater Off | A16 disables output and rejects remote/automatic enable while Off | Software command, not isolation; seasonal shutdown uses breakers/disconnects |
| Auto | A16 controls enable through the approved interface | Defaults, permissives and recovery policy |
| Flush request | Run a bounded flush with an absolute timeout; initially one valve at a time | Pressure requirement, duration/volume, cooldown and daily cap |
| UV cooling request | Periodic timed refresh flushes; no temperature sensor planned | Set interval and bounded duration from existing effective operation and installed-unit guidance; verify actual cooling and that takeoff sweeps UV chamber. No verified flow signal to reset an inactivity timer. |
| UV abnormal electrical reading | Alarm on deviation from commissioned startup/run/off baselines | Thresholds, warmup allowance, delay and response |
| Low UV intensity/manufacturer fault | Handle separately from electrical consumption | Available OEM outputs; alarm versus water isolation |
| Loss of network/Home Assistant | Local controls and timeouts continue | Bench-check ESPHome network/API reboot settings and behavior |
| Controller restart | Flush outputs inactive; do not replay an interrupted flush | Pump/heater automatic restart and UV restoration policy |
| Lost DC control power | Unpowered normally-open contactors release and normally-closed valves close if mechanically functional | Repair/replace supply; no failover requirement. Controller failure is likewise an accepted repair event. |
| Invalid/stale pressure/flow/meter data | Flag unavailable; inhibit routines that require valid data | Do not translate missing readings into real zero values |
| Suspected leak | Optional alarm-only input proposed; no automatic shutdown planned | Under-cabin location, user considers leak consequence limited. Sensor needs a place where escaping water reaches it; no main isolation valve selected. |
| Winterization/service | Existing appropriate breakers/disconnects provide positive shutdown; do not rely on selector Off | Map/label feeds, drain sequence and deliberate return to service |

Keep heater thermostat/high-limit and pump pressure/motor protection in place. Pressure sensing alone cannot prove a heater tank is full or provide complete dry-fire protection. Manual enable must not silently bypass essential protections.

Electrical consumption is a useful UV operating signal, but does not verify germicidal intensity, sleeve cleanliness, water UV transmission or treatment dose. Follow the installed UV model's lamp-life, water-quality and flow requirements. VIQUA was general background; the subsequently identified Cactus family manual is the more relevant reference, subject to exact model confirmation. [S05, S06]

## Electrical and enclosure work still required

### Existing enclosure: first layout candidate

Mike confirmed an existing case with a horizontal divider. The latest screenshot introduces a size ambiguity: its purchase banner reports a previously purchased 13 x 9.2 x 5.6-inch variant, the selected size is 18.1 x 12.6 x 6.4 inches at $75.99, and the title says 16.1 x 12.2 x 5.9 inches. This supersedes treating ownership of the original 16.5 x 12.6 x 6.1-inch cart variant as certain. Verify the physical case and selected SKU before choosing a replacement. Provisionally favor the larger roughly 18 x 12 x 6-inch case for wiring/service space; no fit established or purchase selected.

Use the upper DIN rail for the A16/low-voltage equipment and the lower rail for contactors, power supply and mains terminals. Measure usable mounting surfaces and door clearances. The pictured shelf/divider is not by itself proof of an electrical segregation barrier; maintain appropriate protected routing around openings. Pictured vents/fans and any thermostat option remain unverified accessories, not requirements for this build.

Room temperature: Mike already has a Sonoff thermostat controller maintaining the water-closet temperature. Prefer retaining that function and reusing its temperature reading if the installed model/integration permits, with no additional room sensor planned. Room heat is distinct from the domestic hot-water tank load. A future sensor inside the electrical enclosure would monitor cabinet heat rather than room temperature; it is optional and not selected. Exact Sonoff model, heating load and integration remain unrecorded; no change to its working controls is authorized by this discussion.

- Low-voltage area: A16 with its proper DIN carrier/case, CT conditioning/ADC beside the A16, existing 12-to-24V converter if used, sensor terminals and a compact protected DC distribution point.
- Mains area: three contactors, AC input side of the isolated DIN power supply, circuit terminals/protection, protective-earth connections and CTs around individual load conductors. Route the supply's DC output and the contactors' coil wiring through controlled divider crossings.
- Use the horizontal divider for upper low-voltage/lower mains separation, with protected and organized wiring crossings.
- Prefer the owned low-voltage distribution device if suitable, or a few bridged DIN terminals. A16 outputs switch compatible loads; they are not a substitute for general supply distribution and branch protection. Direct coil/valve driving remains conditional on exact output, coil-current and suppression compatibility. Manufacturer hardware reference: https://www.kincony.com/esp32-board-16-channel-relay-hardware.html
- Reserve wire-routing space, terminal access, Ethernet/USB clearance, CT clearance, door-switch depth and modest expansion room. Check manufacturer spacing and heat requirements before approving the footprint.
- Missing assembly categories: rail end stops/carrier mounts, wire duct, glands/strain relief, terminal covers/markers, suitable conductors/terminations, branch protection, coil suppression and a secure mount for the ADC/converter.

Flow measurement is deferred from the initial build by agreement. Pressure monitoring remains planned. Mike prefers frequent timed UV refresh flushes over temperature sensing; omit the temperature sensor from the initial build. A leak detector remains an optional alarm-only proposal, with no automatic shutdown planned or sensor selected. The water closet is underneath the cabin; placement must reflect where water actually collects or travels. No assumption of a finished floor or collection pan.

### Remaining electrical checks

### Lower-section layout and terminal count

Latest user-supplied dimension drawing for the 16.5 x 12.6 x 6.1-inch variant shows internal width 11.8 inches/299mm, height 14.5 inches/368mm and depth 5.4 inches/138mm. These are listed enclosure interior dimensions, not measured backplate dimensions or guaranteed clearance behind door controls. They do not establish dimensions of the separate 18.1-inch variant. A full 12-inch/304.8mm rail exceeds the depicted interior width; Mike has now selected 11-inch/279.4mm rails, replacing the 12-inch stock. The nominal remaining width is 19.6mm total, about 9.8mm per side if centered; mounting-surface obstructions remain to check. With ground and neutral bars below the rail, the case remains a plausible fit, although the upper end of the component allowance is tight. Confirm exact component widths and mounting surface before cutting/drilling.

Rail-length ballpark (planning allowances, not verified exact-product fit): reserve 36mm per contactor, 108mm total, until the selected DC-coil model's width is established. Some CT1 variants are narrower; the family page alone does not identify the selected unit's dimensions. Allow 40-50mm for the unidentified 12V supply; a comparable Mean Well HDR-30 is 35mm wide, for reference only. Ten DK4N terminal bodies occupy 61mm (6.1mm each), plus approximately 20-30mm for assembly end hardware. Allow another 25-40mm for inter-device spacing/connection access, subject to actual manufacturer clearances. Total planning envelope: 254-289mm, approximately 10-11.4 inches; The selected 11-inch/279.4mm rail lies within that estimate range; verify actual component widths before declaring the complete row fits.

Mike now prefers DIN-mounted ground and neutral blocks, superseding the backplate-mounted bar recommendation. Exact blocks and occupied rail widths are unselected; do not apply the old 83mm bar dimensions to a different DIN product. The earlier 10-11.4-inch rail estimate excludes these new blocks and must be expanded by their actual widths. Use a second short rail within the lower compartment if needed. Neutral must remain insulated from the rail/earth; any PE terminal that relies on the rail for grounding requires a manufacturer-approved rail material and bonding arrangement (selected rails are aluminum). Space for CTs, cable bends and any additional protective devices remains necessary.

Terminal-dimension clarification from Mike's latest screenshot: the product description specifies 6.1 x 39.6 x 40.35mm for individual DK4N blocks and says ten separate circuits are assembled with two SS2 end brackets and one end cover on a five-inch aluminum rail. Ten block bodies therefore occupy 61mm/2.40 inches along the rail, plus end hardware. The metadata's 1.59 x 1.56-inch figures match the individual cross-sectional dimensions, not the ten-block row length. If retaining the supplied mini-rail, reserve five inches for that assembly; if transferring blocks/end hardware to the selected 11-inch rail, use the actual assembled width. Do not stack the supplied five-inch rail on the 11-inch rail.

Dimension references: Dinkle DK4N https://www.dinkle.com/ksen/terminal/DK4N ; reference supply https://www.meanwell.com/Upload/PDF/HDR-30/HDR-30-SPEC.PDF ; Heschen family page (not exact selected-coil dimensions) https://heschen.com/products/ct1-25-2p-12v . Bar dimensions are from Mike's screenshot. No contactor or power-supply substitution is selected by this estimate.

Mike wants all external AC incoming/outgoing cables landed on terminal blocks, with short internal leads to the contactors. Adopt this serviceable arrangement. Conceptual equipment row: pump contactor, heater contactor, UV contactor (provisional load assignment), isolated 12V supply. Put labeled field terminals toward cable entry, with neutral and protective-earth connections separate. If the terminals crowd the equipment rail, use a short separate terminal rail within the lower compartment rather than compressing wiring space.

| External hot-conductor group | Isolated feed-through terminal positions |
|---|---:|
| Pump 240V: L1/L2 incoming and L1/L2 outgoing | 4 |
| Heater 240V: L1/L2 incoming and L1/L2 outgoing | 4 |
| UV 120V circuit: line incoming and switched line outgoing | 2 |
| Base total, excluding neutral and earth | 10 |

Mike confirmed ten hot-conductor terminal positions as the fixed count, excluding neutral and earth; do not add spare positions to that count. These are ten electrically separate positions, not a ten-way common bus. Each feed-through connects a field conductor to its internal lead. Provide the control-supply line branch with a suitable multi-connection incoming 120V terminal or a rated internal splice; do not assume the two-connection DK4N accepts multiple wires under one screw. The ten-position Dinkle cart assembly therefore still needs its branch-connection method resolved, without increasing the agreed external terminal count.

Incoming/outgoing cable plan: interpret the pump/heater description as four cable runs total (one feed and one load cable for each 240V circuit), pending physical wiring identification. Mike suspects the pump cable is 12/2 rather than 12/3. In NM-B nomenclature, 12/2 with ground has two insulated conductors plus ground; 12/3 with ground has three plus ground. Neither the neutral requirement nor exact cable size is established by that recollection. Standard NM-B is for dry locations; confirm the under-cabin cable environment before selecting new cable. Southwire reference: https://www.southwire.com/wire-cable/building-wire/romex-sup-sup-brand-simpull-sup-sup-copper-type-nm-b-cable/p/28828280

Mike proposes using the male and female portions of a cut grounded extension cord as the panel's 120V input and switched UV output respectively. Record the functional direction as selected, with actual cord/connection method pending suitability: incoming 120V branches before the UV contactor to supply the controller continuously; only the UV branch is switched. UV neutral stays with that 120V source, and protective earth remains continuous. Cord type, conductor ampacity/protection, environment, permitted equipment-cord use, fine-strand terminations and proper cord grips/strain relief must be established; a cut consumer cord is not automatically an approved finished assembly. Flexible leads must not substitute for fixed building wiring. General cord/strain-relief reference: https://www.osha.gov/laws-regs/regulations/standardnumber/1910/1910.305 (workplace reference, not a determination of the cabin's local installation requirements). Unplugging the 120V input does not isolate the two 240V circuits.

Also allocate physical space for three CTs (pump, heater, UV) around individual load conductors, required protection, end stops/covers and cable bends. Neutral allocation is three connection points for the assumed shared 120V source/UV/control-supply circuit, with any actual pump neutral kept on its own circuit's isolated terminals. Ground count and bonding remain as described below. This is a functional layout and count, not a construction wiring diagram or final rail-fit determination.

Ground/neutral planning update: Mike added and subsequently re-presented the same Bonsicoky two-pack of six-position brass terminal bars, shown at $5.99, as the intended isolated-mount approach. Blue insulating bases appear to isolate each brass bar from its mounting surface; this is not a verified insulation rating. Neutral remains separate from earth. A ground bar may also use an insulating mount provided the incoming protective-earth conductors connect directly to it and any required metalwork bonding uses dedicated connections, not an assumed DIN clip contact. Screenshot metadata says DIN Rail Mount, but the visible feet have screw holes and no underside clip is shown: confirm actual rail attachment before assigning its mounted footprint. Listed length remains 83mm per bar; reusing these same bars does not gain the smaller footprint of compact DIN terminals. The screenshot does not establish conductor range, tightening torque, mains/grounding approval or current rating; retain as a candidate pending those specifications. All six positions on each bar are electrically common.

With three incoming circuits (120V, pump 240V, heater 240V), a common protective-earth connection is appropriate. If all three load cables also leave this enclosure, allow six external ground terminations: three incoming plus pump, heater and UV outgoing. Add any required power-supply protective earth and metal backplate/rail bonding connections; a six-position bar may therefore be too small. Do not assume multiple wires are permitted under one screw.

Keep neutral separate from protective earth in this control enclosure. Also keep neutrals belonging to separately supplied branch circuits separate from one another here. The 120V supply neutral may serve the UV and control power supply when both are powered by that same circuit (three neutral terminations before other loads). A pump neutral, if actually required, needs its own isolated incoming/outgoing connection, not the 120V common bar. A straight 240V motor often has no neutral; identify the actual pump/control wiring before assigning conductors. Source: https://www.se.com/us/en/faqs/FA124419/ and shared-neutral protection discussion https://www.se.com/us/en/faqs/FA109025/ .

- Record every incoming circuit, disconnect, breaker and conductor arrangement, including whether 120V control power and 240V loads have separate feeds. Provide a clear all-sources service-isolation scheme.
- Match contactors to load voltage, heater duty and motor HP/starting duty. Confirm coil pickup/holding demand and rated inductive switching at controller outputs. Selectors carry input signals only.
- Budget DC demand from the controller, all simultaneously allowed coils/valves, sensors, indicators and converters. Account for startup, margin and enclosure temperature; 2A is currently unproven.
- Keep supply mains/low-voltage isolation and wiring separation clear; size branch protection, terminals, conductors and bonding for the final design. Neither red/black hobby-wire marketing nor a DIN-rail mount establishes mains suitability.
- Verify actual enclosure interior/backplate dimensions, door-switch depth, cable bends, CT clearance, rail lengths, heat and condensation management. Mike selected 11-inch rails; verify mounting points and actual usable rail length.
- Provide protected distribution, suitable grounds/neutrals, wire identification, end stops, covers, cable glands/strain relief and appropriate coil suppression. A distribution board does not replace circuit protection.
- Have the completed mains design and installation checked by the installer responsible for the cabin electrical work before energizing. This record does not prescribe construction wiring.

References are defined in SOURCES.md. No specific firmware thresholds or prior proposed GPIO assignments are approved for installation.
