# Cabin water closet

ESPHome controller for a cabin water system on one KinCony KC868-A16. It runs the well pump, tank heater, UV lamp, spin-filter flush, and system flush. Local selectors and buttons stay usable, and a small dashboard shows mode, flush time, and the UV start countdown.

Power restoration starts outputs off and restores the saved Normal/Away/Shutdown mode through the selectors and interlocks. Mode changes commit immediately to flash. The heater waits five seconds after each pump enable; its LED slowly blinks and the dashboard explains the startup wait. This is sequencing only: the tank must already be filled. A future CT-based pump-cycle startup idea is recorded in DESIGN.md.

Operational firmware is [water-closet.yaml](water-closet.yaml). The `kc868-a16*.yaml` files are bench pages. Modes are **Normal, Away, and Shutdown**. The controller owns local operation; Home Assistant integration is later work.

## Secrets

Wi-Fi credentials, the API encryption key, and the OTA password are not in this repository. The YAML files reference them with `!secret`. Copy [secrets.yaml.example](secrets.yaml.example) to `secrets.yaml` and fill in the local values. `secrets.yaml` is gitignored, as is the `.esphome/` build tree, which contains those values after a compile.

## What it does

The dashboard puts mode and equipment status first. Manual controls stay visible. Wiring tools are under collapsed **Advanced → Bench test**. Normal and Away requests from the web or API require both selectors in Auto. Shutdown is always accepted. Moving both selectors to Auto requests local Normal; both Off requests Shutdown. Leaving the selectors in Auto preserves a selected Away mode.

Bench testing provides independent OUT1–16 toggles and live IN1–16 indicators. Automatic rules are bypassed only during that session. **All outputs off** clears every output. Leaving the test returns to Shutdown with all outputs off. A test session does not restore after reboot. Dashboard sources are `web/dashboard.js` and `web/dashboard.css`. Firmware endpoints are in `firmware/bench.yaml`.

Away has separate system-water-exchange and spin-flush schedules, each with enable, one-to-four runs per day, and duration. Each run temporarily enables the Auto pump with the tank and UV off. If both are due, spin runs first. Door light patterns are in [DOOR_CONTROLS.md](DOOR_CONTROLS.md).

Current-transformer sampling and UV current monitoring are implemented and left off until the sensors are installed and calibrated. Missing sensors stay unknown. They are not treated as a dead-lamp alarm. See [DESIGN.md](DESIGN.md) and `firmware/health.yaml`.

Contactor outputs are OUT1–OUT3, valve outputs OUT4–OUT5, and LED outputs OUT9–OUT12. See [CONNECTIONS.md](CONNECTIONS.md). Equipment and sensor commissioning is still open. CT sampling stays off, and an unavailable pressure reading is expected until the sensor is installed.

## Build and test

```text
esphome compile water-closet.yaml
tests/run.ps1
```

Controller tests cover UV countdown, LED timing, bench-session reset, remote-mode guards, local mode changes, pump-off heater lockout, flush limits, independent Away schedules, UV current-fault grace/recovery, missing samples, activity counters, and timer rollover.

The October 6 build at **11:15:02 CDT** was compiled, flashed and checked through the encrypted API and live dashboard. Away and the saved settings were preserved. Both next-run displays were tested, then both schedules were restored to disabled. CT sampling and UV monitoring remain off. See DESIGN.md for deployment details and the remaining physical checks.

## Documents

- [Design and operation](DESIGN.md)
- [Connection schedule](CONNECTIONS.md)
- [Bench wiring check](WIRING.md)
- [Door controls and indication](DOOR_CONTROLS.md)
- [Decisions](DECISIONS.md)
- [Parts](PARTS.md)
- [Optional flow measurement](FLOW_OPTIONS.md)
- [Sources](SOURCES.md)
