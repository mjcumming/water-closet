#include "../water_closet_control.h"
#include "../water_closet_monitoring.h"
#include <cassert>
#include <cstdio>
#include <initializer_list>
#include <limits>
#include <cstring>

using namespace water_closet;

struct Rig {
  Controller controller;
  Inputs in{Mode::NORMAL, Selector::AUTO, Selector::AUTO, true};
  Settings settings;
  Outputs at(uint32_t now, Request request = Request::NONE) {
    return controller.step(now, in, settings, request);
  }
};

int main() {
  { // Every pump enable, including boot/rollover, starts a fresh heater delay.
    for (uint32_t start : {0u, 0xfffffc00u}) {
      for (auto selector : {Selector::AUTO, Selector::ON}) {
        Rig r; r.in.tank = selector;
        auto out = r.at(start);
        assert(out.pump && !out.tank && out.tank_pending && !out.tank_blocked);
        assert(!r.at(start + 4999).tank);
        assert(r.at(start + 5000).tank && !r.controller.outputs().tank_pending);
        r.in.pump = Selector::OFF;
        out = r.at(start + 5100);
        assert(!out.pump && !out.tank && !out.tank_pending && out.tank_blocked);
        r.in.pump = Selector::AUTO;
        assert(r.at(start + 5200).tank_pending);
        assert(!r.at(start + 10199).tank);
        assert(r.at(start + 10200).tank);
      }
    }
  }
  { // Off/Shutdown/invalid input/readiness loss cancel, with no delayed replay.
    for (unsigned stop = 0; stop < 5; ++stop) {
      Rig r; r.at(0);
      if (stop == 0) r.in.mode = Mode::SHUTDOWN;
      if (stop == 1) r.in.mode = Mode::AWAY;
      if (stop == 2) r.in.pump = Selector::INVALID;
      if (stop == 3) r.in.tank = Selector::OFF;
      if (stop == 4) r.in.ready = false;
      assert(!r.at(1000).tank_pending);
      assert(!r.at(6000).tank);
      r.in = {Mode::NORMAL, Selector::AUTO, Selector::AUTO, true};
      const auto resumed = r.at(7000);
      // Tank-only Off leaves pump enabled; its completed startup delay still counts.
      assert(resumed.tank == (stop == 3));
      assert(resumed.tank_pending == (stop != 3));
      assert(r.at(12000).tank);
    }
  }
  { // A reboot forgets elapsed pump time; Away/Shutdown boot stays idle.
    Rig r; r.at(0); assert(r.at(5000).tank);
    r.controller = Controller{};
    assert(r.at(0).tank_pending && !r.controller.outputs().tank);
    assert(!r.at(4999).tank && r.at(5000).tank);
    for (auto mode : {Mode::AWAY, Mode::SHUTDOWN}) {
      r.controller = Controller{}; r.in.mode = mode;
      auto out = r.at(0);
      assert(!out.pump && !out.tank && !out.tank_pending);
    }
    Outputs pending; pending.tank_pending = true;
    Inputs in{Mode::NORMAL, Selector::AUTO, Selector::AUTO, true};
    assert(indicators(0, in, pending, true).tank);
    assert(!indicators(500, in, pending, true).tank);
  }
  { // Continuous enable across a full millis cycle never restarts heater waiting.
    Rig r; r.settings.spin_auto = r.settings.system_auto = false;
    r.at(1000); assert(r.at(6000).tank);
    assert(r.at(0xfffffff0u).tank);
    assert(r.at(1000).tank && !r.controller.outputs().tank_pending);
  }
  { // Missing CTs and zero calibration are unknown, never low-current alarms.
    CurrentSample sample;
    assert(std::isnan(sample.amps(0, 10, true)));
    sample.record(0xffffff00u, 0.25f);
    assert(sample.amps(0xffffff00u + 1000, 10, true) == 2.5f);
    assert(std::isnan(sample.amps(0xffffff00u + 15001, 10, true)));
    assert(std::isnan(sample.amps(0xffffff00u, 0, true)));
    assert(std::isnan(sample.amps(0xffffff00u, 10, false)));
    sample.record(100, NAN); assert(std::isnan(sample.amps(100, 10, true)));
    UVHealth uv;
    uv.update(0, false, true, 0, 0.2f, 60000, 30000); assert(!uv.fault());
    uv.update(100000, true, true, NAN, 0.2f, 60000, 30000); assert(!uv.fault());
    uv.update(200000, true, true, 0, 0, 60000, 30000); assert(!uv.fault());
  }
  { // Warmup belongs to each lamp start; sustained low current, recovery, stale data.
    UVHealth uv;
    const uint32_t base = 0xfffffc00u;
    uv.update(base, true, true, 0, 0.2f, 2000, 1000);
    uv.update(base + 1999, true, true, 0, 0.2f, 2000, 1000); assert(!uv.fault());
    uv.update(base + 2000, true, true, 0, 0.2f, 2000, 1000); assert(!uv.fault());
    uv.update(base + 2999, true, true, 0, 0.2f, 2000, 1000); assert(!uv.fault());
    uv.update(base + 3000, true, true, 0, 0.2f, 2000, 1000); assert(uv.fault());
    uv.update(base + 3100, true, true, NAN, 0.2f, 2000, 1000); assert(!uv.fault());
    uv.update(base + 3200, true, true, 0, 0.2f, 2000, 1000); assert(!uv.fault());
    uv.update(base + 4200, true, true, 0, 0.2f, 2000, 1000); assert(uv.fault());
    uv.update(base + 4300, true, true, 0.3f, 0.2f, 2000, 1000); assert(!uv.fault());
    uv.update(base + 4400, true, false, 0, 0.2f, 2000, 1000);
    uv.update(base + 4500, true, true, 0, 0.2f, 2000, 1000);
    uv.update(base + 6000, true, true, 0, 0.2f, 2000, 1000);
    assert(!uv.fault() && std::strcmp(uv.status(), "Warming up") == 0);
    Inputs in{Mode::NORMAL, Selector::AUTO, Selector::AUTO, true};
    Outputs out; out.uv = out.system = true;
    assert(indicators(0, in, out, false, true, true).uv_system);
    assert(!indicators(100, in, out, false, true, true).uv_system);
    out.system = false;
    assert(indicators(200, in, out, false, true, true).uv_system); // Fault wins over network gap.
  }
  { // Usage counts observed off-to-on edges, with hysteresis and unknown gaps.
    UsageMonitor usage;
    usage.update(0, 2, 1); assert(usage.starts() == 0 && usage.running());
    usage.update(1000, 2, 1); assert(usage.runtime_seconds() == 1);
    usage.update(2000, 0.85f, 1); assert(usage.running());
    usage.update(3000, 0.7f, 1); assert(!usage.running());
    usage.update(4000, 1.1f, 1); assert(usage.starts() == 1);
    const auto saved = usage.runtime_seconds();
    usage.update(5000, NAN, 1); assert(!usage.known() && usage.runtime_seconds() == saved);
    usage.update(6000, 2, 1); assert(usage.starts() == 1); // No invented start after missing data.
    usage.update(16000, 2, 1); assert(usage.runtime_seconds() == saved); // No accounting over a long stall.
    usage.update(17000, 2, 0); assert(!usage.known());
    UsageMonitor wrap;
    wrap.update(0xfffffc00u, 2, 1); wrap.update(0xfffffc00u + 1500, 2, 1);
    assert(wrap.runtime_seconds() == 1);
  }
  { // Independent Away schedules serialize, own the pump, and keep heat/UV off.
    Rig r;
    r.in.mode = Mode::AWAY;
    r.settings.away_auto = r.settings.away_spin_auto = true;
    r.settings.away_interval_ms = r.settings.away_spin_interval_ms = 10000;
    r.settings.away_duration_ms = 2000; r.settings.away_spin_duration_ms = 1000;
    r.at(0);
    assert(r.controller.away_next_ms(true, 1000) == 9000);
    auto first = r.at(10000);
    assert(first.pump && !first.tank && !first.uv && !first.spin && !first.system);
    assert(r.controller.active_flush() == Flush::AWAY_SPIN);
    assert(std::strcmp(r.controller.away_schedule_status(false, 10000), "Due: waiting for other flush") == 0);
    assert(r.at(11000).spin);
    assert(!r.at(12000).pump);
    assert(!r.at(12999).pump);
    assert(r.at(13000).pump && r.controller.active_flush() == Flush::AWAY);
    assert(r.at(14000).system);
    assert(!r.at(16000).pump);
    assert(r.controller.away_next_ms(true, 16000) == 4000);
    assert(r.controller.away_next_ms(false, 16000) == 7000);
  }
  { // Enabling/frequency changes start a full interval; mode/reboot reset it.
    Rig r; r.in.mode = Mode::AWAY; r.at(0); r.at(100000);
    assert(std::strcmp(r.controller.away_schedule_status(true, 100000), "Disabled") == 0);
    r.settings.away_spin_auto = true; r.settings.away_spin_interval_ms = 10000;
    r.at(101000); assert(r.controller.away_next_ms(true, 101000) == 10000);
    r.at(105000); r.settings.away_spin_interval_ms = 20000;
    r.at(106000); assert(r.controller.away_next_ms(true, 106000) == 20000);
    r.settings.away_spin_duration_ms = 9000;
    r.at(107000); assert(r.controller.away_next_ms(true, 107000) == 19000);
    r.in.pump = Selector::OFF; r.at(108000);
    assert(std::strcmp(r.controller.away_schedule_status(true, 108000), "Paused: pump selector Off") == 0);
    assert(!r.at(130000).pump);
    r.in.mode = Mode::NORMAL; r.at(131000);
    assert(std::strcmp(r.controller.away_schedule_status(true, 131000), "Paused: select Away") == 0);
    r.in.mode = Mode::AWAY; r.in.pump = Selector::AUTO; r.at(132000);
    assert(r.controller.away_next_ms(true, 132000) == 20000);
    r.controller = Controller{}; r.at(0);
    assert(r.controller.away_next_ms(true, 0) == 20000 && !r.controller.outputs().pump);
  }
  { // Away spin obeys bounded duration, button cancellation, Off and mode changes.
    Rig r; r.in.mode = Mode::AWAY; r.settings.away_spin_duration_ms = 0xffffffffu;
    r.at(0); assert(r.at(1000, Request::SPIN).pump);
    assert(r.at(2000).spin);
    assert(r.at(3000, Request::SYSTEM).spin); // Opposite button never cancels spin.
    assert(!r.at(302000).pump); // Five-minute cap, despite oversized input.
    r.at(303000, Request::SPIN); r.at(304000);
    assert(!r.at(305000, Request::SPIN).pump);
    r.at(306000, Request::SPIN); r.at(307000);
    r.in.pump = Selector::OFF; assert(!r.at(308000).spin);
    r.in.pump = Selector::AUTO; assert(!r.at(309000).pump);
    r.at(310000, Request::SPIN); r.at(311000);
    r.in.mode = Mode::NORMAL;
    auto normal = r.at(312000); assert(normal.pump && normal.tank_pending && !normal.spin);
    assert(r.at(315000).tank);
  }
  { // Disabled schedules do not run; no dependency on wall time or network.
    for (bool spin : {false, true}) {
      Rig r; r.in.mode = Mode::AWAY;
      r.settings.away_spin_auto = spin; r.settings.away_auto = !spin;
      r.settings.away_spin_interval_ms = r.settings.away_interval_ms = 2000;
      const uint32_t start = 0xfffffc00u;
      r.at(start); assert(r.controller.away_next_ms(spin, start + 1000) == 1000);
      assert(r.at(start + 2000).pump);
      assert(r.controller.active_flush() == (spin ? Flush::AWAY_SPIN : Flush::AWAY));
      r.in.ready = false;
      auto stopped = r.at(start + 2100);
      assert(!stopped.pump && !stopped.spin && !stopped.system);
      assert(std::strcmp(r.controller.away_schedule_status(spin, start + 2100), "Paused: control inputs") == 0);
    }
  }
  { // UV LED distinguishes waiting, enabled, and system flushing.
    Inputs in{Mode::NORMAL, Selector::AUTO, Selector::AUTO, true};
    Outputs out;
    assert(!indicators(0, in, out, true).uv_system);
    out.pump = out.tank = true;
    out.uv_pending = true;
    auto led = indicators(0, in, out, true);
    assert(led.pump && led.tank && !led.spin && led.uv_system);
    assert(indicators(300, in, out, true).uv_system);
    assert(!indicators(500, in, out, true).uv_system);
    out.uv_pending = false; out.uv = true;
    assert(indicators(750, in, out, true).uv_system);
    out.system = true;
    assert(indicators(0, in, out, true).uv_system);
    assert(!indicators(300, in, out, true).uv_system);
    // A system flush still indicates itself during Away, with UV intentionally off.
    out.uv = false;
    assert(indicators(500, in, out, true).uv_system);
    assert(!indicators(750, in, out, true).uv_system);
    out.system = false; out.spin = true;
    assert(indicators(500, in, out, true).spin);
    assert(!indicators(750, in, out, true).spin);
    out.spin = false;
    assert(indicators(750, in, out, true, true).spin);
    in.mode = Mode::SHUTDOWN;
    assert(!indicators(750, in, out, true, true).spin);
  }
  { // Wi-Fi failure uses the shared double flash; either active valve wins.
    Inputs in{Mode::NORMAL, Selector::AUTO, Selector::AUTO, true};
    Outputs out; out.pump = out.tank = out.uv = true;
    for (uint32_t t : {0u, 199u, 400u, 599u, 5000u}) {
      auto led = indicators(t, in, out, false);
      assert(led.pump && led.tank && led.spin && led.uv_system);
    }
    for (uint32_t t : {200u, 399u, 600u, 4999u}) {
      auto led = indicators(t, in, out, false);
      assert(led.pump && led.tank && !led.spin && !led.uv_system);
    }
    out.spin = true;
    auto spin = indicators(700, in, out, false);
    assert(spin.spin && spin.uv_system);
    out.spin = false; out.system = true;
    auto system = indicators(300, in, out, false);
    assert(!system.spin && !system.uv_system);
    out.system = false;
    assert(indicators(700, in, out, true).uv_system); // Connection restores UV status.
    out.pump = out.tank = false; out.tank_blocked = true;
    assert(indicators(0, in, out, true).tank);
    assert(!indicators(500, in, out, true).tank);
    in.pump = in.tank = Selector::INVALID;
    assert(indicators(0, in, out, true).pump && indicators(0, in, out, true).tank);
    assert(!indicators(300, in, out, true).pump && !indicators(300, in, out, true).tank);
  }
  { // Countdown follows the real off timer, including cancellation and rollover.
    Rig r;
    assert(r.at(0).uv_pending);
    assert(r.controller.uv_wait_remaining_ms(0) == 300000);
    assert(r.controller.uv_wait_remaining_ms(12345) == 287655);
    assert(r.at(300000).uv && r.controller.uv_wait_remaining_ms(300000) == 0);
    r.in.pump = Selector::OFF;
    r.at(300100);
    assert(r.controller.uv_wait_remaining_ms(300100) == 0);
    r.in.pump = Selector::AUTO;
    r.at(301100);
    assert(r.controller.uv_wait_remaining_ms(301100) == 299000);
    r.in.mode = Mode::SHUTDOWN; r.at(301200);
    assert(r.controller.uv_wait_remaining_ms(301200) == 0);
    Rig wrap;
    wrap.settings.uv_min_off_ms = 5000;
    const uint32_t start = 0xfffffc00u;
    wrap.at(start);
    assert(wrap.controller.uv_wait_remaining_ms(start + 2000) == 3000);
    assert(wrap.at(start + 5000).uv);
    assert(wrap.controller.uv_wait_remaining_ms(start + 5000) == 0);
  }
  { // Direct wiring tests have explicit ownership and never restore stale outputs.
    BenchTest bench;
    assert(!bench.active() && bench.mask() == 0);
    assert(!bench.set(1, true));
    bench.begin();
    assert(bench.set(2, true) && bench.mask() == 2); // Individual heater coil test.
    assert(!bench.set(0, true) && !bench.set(17, true));
    for (unsigned channel = 1; channel <= 16; ++channel) assert(bench.set(channel, true));
    assert(bench.mask() == 0xffff);
    assert(bench.set(16, false) && bench.mask() == 0x7fff);
    bench.all_off();
    assert(bench.active() && bench.mask() == 0);
    bench.set(9, true);
    bench.end();
    assert(!bench.active() && bench.mask() == 0 && !bench.set(9, true));
    bench.begin();
    assert(bench.mask() == 0);
    bench.set(1, true);
    bench = BenchTest{};
    assert(!bench.active() && bench.mask() == 0);
  }
  { // Remote operation needs both selectors Auto; Shutdown is always accepted.
    for (auto pump : {Selector::OFF, Selector::ON, Selector::AUTO, Selector::INVALID})
      for (auto tank : {Selector::OFF, Selector::ON, Selector::AUTO, Selector::INVALID})
        for (bool ready : {false, true})
          for (bool bench : {false, true}) {
            Inputs in{Mode::SHUTDOWN, pump, tank, ready};
            bool allowed = ready && !bench && pump == Selector::AUTO && tank == Selector::AUTO;
            assert(remote_mode_allowed(Mode::NORMAL, in, bench) == allowed);
            assert(remote_mode_allowed(Mode::AWAY, in, bench) == allowed);
            assert(remote_mode_allowed(Mode::SHUTDOWN, in, bench));
          }
  }
  // All selector/mode combinations: local Off, invalid, and Shutdown win.
  for (auto mode : {Mode::NORMAL, Mode::AWAY, Mode::SHUTDOWN}) {
    for (auto pump : {Selector::OFF, Selector::ON, Selector::AUTO, Selector::INVALID}) {
      for (auto tank : {Selector::OFF, Selector::ON, Selector::AUTO, Selector::INVALID}) {
        Rig r;
        r.in = {mode, pump, tank, true};
        auto out = r.at(0);
        const bool pump_expected = mode != Mode::SHUTDOWN &&
            (pump == Selector::ON || (pump == Selector::AUTO &&
             mode == Mode::NORMAL));
        assert(out.pump == pump_expected);
        assert(!out.tank);
        out = r.at(Controller::TANK_START_DELAY_MS);
        assert(out.tank == (pump_expected &&
            (tank == Selector::ON || (tank == Selector::AUTO && mode == Mode::NORMAL))));
        assert(!out.spin && !out.system && !out.uv);
      }
    }
  }
  assert(decode(false, false) == Selector::OFF);
  assert(decode(true, false) == Selector::ON);
  assert(decode(false, true) == Selector::AUTO);
  assert(decode(true, true) == Selector::INVALID);

  { // Local start/stop never turns an unchanged Away installation back on at boot.
    LocalSelectors local;
    Inputs in{Mode::AWAY, Selector::AUTO, Selector::AUTO, true};
    Mode requested = Mode::AWAY;
    assert(!local.request(in, requested));
    assert(!local.request(in, requested));
    in.tank = Selector::OFF;
    assert(!local.request(in, requested)); // No inferred Away from cold-water-only.
    in.pump = Selector::OFF;
    assert(local.request(in, requested) && requested == Mode::SHUTDOWN);
    in.pump = Selector::AUTO;
    assert(local.request(in, requested) && requested == Mode::NORMAL);
    in.tank = Selector::AUTO;
    assert(local.request(in, requested) && requested == Mode::NORMAL);
    in.pump = Selector::INVALID;
    assert(!local.request(in, requested));
    in.pump = Selector::AUTO;
    assert(!local.request(in, requested)); // Invalid input recovery is not a gesture.
  }
  { // Indication reports inhibited heating, not intentional mode shutdown.
    Rig r;
    r.in.pump = Selector::OFF;
    assert(r.at(0).tank_blocked);
    r.in.tank = Selector::ON;
    assert(r.at(100).tank_blocked);
    r.in.mode = Mode::SHUTDOWN;
    assert(!r.at(200).tank_blocked);
    r.in.mode = Mode::AWAY;
    r.in.pump = r.in.tank = Selector::AUTO;
    assert(!r.at(300).tank_blocked);
    r.in.mode = Mode::NORMAL;
    assert(r.at(400).tank_pending && !r.at(400).tank_blocked);
    assert(r.at(5400).tank);
  }

  { // Local Away schedule needs no clock or HA; defaults do not auto-flush.
    Rig r;
    r.in.mode = Mode::AWAY;
    r.at(0);
    assert(!r.at(43200000).pump);
    r.settings.away_auto = true;
    assert(!r.at(43200100).pump);
    assert(!r.at(86400099).pump);
    assert(r.at(86400100).pump);
    assert(r.at(86401100).system);
    assert(!r.at(86701100).pump);
    r.in.mode = Mode::SHUTDOWN;
    assert(!r.at(129600000).pump);
  }

  { // Button cancels, opposite button is ignored, duration changes don't extend.
    Rig r;
    r.at(0);
    assert(r.at(1000, Request::SPIN).spin);
    assert(r.at(2000, Request::SYSTEM).spin);
    r.settings.spin_duration_ms = 300000;
    assert(!r.at(31000).spin);
    assert(r.at(32000, Request::SPIN).spin);
    assert(!r.at(33000, Request::SPIN).spin);
    assert(!r.at(33500, Request::SPIN).spin); // Valve gap, no queue.
    assert(!r.at(34000).spin);
  }
  { // Long scheduled runs finish even when the scheduler is repeatedly due.
    Rig r;
    r.settings.spin_duration_ms = 120000;
    r.settings.spin_interval_ms = 1000;
    r.settings.system_auto = false;
    r.at(0);
    assert(r.at(1000).spin);
    assert(r.at(61000).spin);
    assert(!r.at(121000).spin);
    assert(!r.at(121999).spin);
  }
  { // Bounds cannot be bypassed by large settings or repeated other requests.
    Rig r;
    r.settings.spin_duration_ms = std::numeric_limits<uint32_t>::max();
    r.at(0);
    assert(r.at(1000, Request::SPIN).spin);
    for (uint32_t t = 2000; t < 301000; t += 1000)
      assert(r.at(t, Request::AWAY).spin);
    assert(!r.at(301000).spin);
  }
  { // Due valves serialize; stopping a run resets its automatic interval.
    Rig r;
    r.settings.spin_interval_ms = r.settings.system_interval_ms = 1000;
    r.at(0);
    auto first = r.at(1000);
    assert(first.spin && !first.system);
    assert(!r.at(2000, Request::CANCEL).spin);
    assert(!r.at(2500).system);
  }
  { // Pump Off cancels every valve and inhibits tank/UV regardless of current.
    Rig r;
    r.at(0);
    r.at(1000, Request::SYSTEM);
    r.in.pump = Selector::OFF;
    auto out = r.at(1100);
    assert(!out.pump && !out.tank && !out.uv && !out.spin && !out.system);
    r.in.pump = Selector::AUTO;
    assert(!r.at(1200).system);
  }
  { // Explicit requirement: Tank On never overrides Pump Off, in any mode.
    for (auto mode : {Mode::NORMAL, Mode::AWAY, Mode::SHUTDOWN}) {
      Rig r;
      r.in.mode = mode;
      r.in.pump = Selector::OFF;
      r.in.tank = Selector::ON;
      for (auto request : {Request::NONE, Request::SPIN, Request::SYSTEM, Request::AWAY}) {
        const auto out = r.at(1000, request);
        assert(!out.pump && !out.tank && !out.uv && !out.spin && !out.system);
      }
    }
  }
  { // Away owns the pump only for this run; returning Normal cannot be undone.
    Rig r;
    r.in.mode = Mode::AWAY;
    r.at(0);
    auto starting = r.at(1000, Request::AWAY);
    assert(starting.pump && !starting.system && !starting.uv && !starting.tank);
    assert(r.at(2000).system);
    r.in.mode = Mode::NORMAL;
    auto normal = r.at(3000);
    assert(normal.pump && normal.tank_pending && !normal.tank && !normal.system);
    assert(r.at(6000).tank); // Pump has been continuously enabled since t=1000.
    assert(r.at(302000).pump);
  }
  { // Completed Away run releases Auto pump, but honors a physical On selector.
    for (auto selector : {Selector::AUTO, Selector::ON}) {
      Rig r;
      r.in.mode = Mode::AWAY;
      r.in.pump = selector;
      r.at(0);
      r.at(1000, Request::AWAY);
      r.at(2000);
      assert(r.at(302000).pump == (selector == Selector::ON));
    }
  }
  { // Away exchange has its own longer bound, separate from short flush buttons.
    Rig r;
    r.in.mode = Mode::AWAY;
    r.settings.away_duration_ms = 600000;
    r.at(0);
    r.at(1000, Request::AWAY);
    r.at(2000);
    assert(r.at(302000).system);
    assert(r.at(601999).system);
    assert(!r.at(602000).system);
    r.settings.away_duration_ms = 0xffffffffu;
    r.at(603000, Request::AWAY);
    r.at(604000);
    assert(!r.at(2404000).system);
  }
  { // Shutdown interrupts On and every kind of flush, with no replay on return.
    Rig r;
    r.in.pump = r.in.tank = Selector::ON;
    r.at(0);
    r.at(1000, Request::SPIN);
    r.in.mode = Mode::SHUTDOWN;
    auto winter = r.at(2000);
    assert(!winter.pump && !winter.tank && !winter.spin && !winter.uv);
    r.in.mode = Mode::NORMAL;
    assert(!r.at(3000).spin);
    r.in.ready = false;
    assert(!r.at(4000).pump);
  }
  { // UV waits after boot/off; a withdrawn request cannot energize it later.
    Rig r;
    assert(r.at(0).uv_pending);
    assert(!r.at(299999).uv);
    assert(r.at(300000).uv);
    r.in.pump = Selector::OFF;
    assert(!r.at(300100).uv);
    r.in.pump = Selector::AUTO;
    assert(!r.at(300200).uv);
    assert(!r.at(600099).uv);
    assert(r.at(600100).uv);
    assert(r.controller.uv_on_since() == 600100);
    r.in.mode = Mode::SHUTDOWN;
    assert(!r.at(600200).uv);
    assert(!r.at(999999).uv);
  }
  { // The web UV enable setting is a persistent permission, not a raw bypass.
    Rig r;
    r.at(0);
    assert(r.at(300000).uv);
    r.settings.uv_enabled = false;
    assert(!r.at(300100).uv);
    assert(!r.at(900000).uv);
    r.settings.uv_enabled = true;
    assert(r.at(900100).uv);
    r.in.mode = Mode::SHUTDOWN;
    assert(!r.at(900200).uv);
  }
  { // Timer arithmetic survives the 49.7-day millis rollover.
    Rig r;
    const uint32_t start = 0xfffffc00u;
    r.at(start);
    assert(r.at(start + 1000, Request::SPIN).spin);
    assert(r.at(start + 2000).spin);
    assert(!r.at(start + 31000).spin);
    assert(r.at(start + 300000).uv);
  }
  { // Restart never restores a valve or an Away pump run.
    Rig r;
    r.in.mode = Mode::AWAY;
    r.at(0);
    r.at(1000, Request::AWAY);
    r.at(2000);
    r.controller = Controller{};
    auto reboot = r.at(0);
    assert(!reboot.pump && !reboot.system && !reboot.spin);
  }
  { // Exercise mixed events across timer rollover and many mode transitions.
    Rig r;
    uint32_t random = 1977;
    uint32_t now = 0xffff0000u;
    for (int i = 0; i < 50000; ++i) {
      random = random * 1664525u + 1013904223u;
      now += 1000 + (random & 0xffff);
      r.in.mode = static_cast<Mode>((random >> 16) % 3);
      r.in.pump = static_cast<Selector>((random >> 18) % 4);
      r.in.tank = static_cast<Selector>((random >> 20) % 4);
      r.in.ready = (random & 7) != 0;
      auto out = r.at(now, static_cast<Request>((random >> 22) % 5));
      assert(!(out.spin && out.system));
      if (!r.in.ready || r.in.mode == Mode::SHUTDOWN)
        assert(!out.pump && !out.tank && !out.uv && !out.spin && !out.system);
      if (r.in.pump == Selector::OFF || r.in.pump == Selector::INVALID)
        assert(!out.pump);
      if (r.in.tank == Selector::OFF || r.in.tank == Selector::INVALID)
        assert(!out.tank);
      if (out.spin || out.system) assert(out.pump);
      if (out.tank) assert(out.pump && !out.tank_pending && !out.tank_blocked);
      if (r.controller.away_flush()) assert(!out.tank && !out.uv);
    }
  }
  std::puts("PASS: heater startup/cancellation, selectors, bench, dual Away schedules, LED priorities, UV fault/unknown/grace, CT usage, rollover and restart");
}
