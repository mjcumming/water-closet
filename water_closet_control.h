#pragma once

// Hardware-independent controller. ESPHome calls step() every 100 ms.
// Keep timers here: no delayed script is allowed to write equipment outputs.
#include <cstdint>

namespace water_closet {

enum class Mode { NORMAL, AWAY, SHUTDOWN };
enum class Selector { OFF, ON, AUTO, INVALID };
enum class Flush { NONE, SPIN, SYSTEM, AWAY, AWAY_SPIN };
enum class Request { NONE, SPIN, SYSTEM, AWAY, CANCEL };

inline Selector decode(bool on, bool automatic) {
  if (on && automatic) return Selector::INVALID;
  return on ? Selector::ON : automatic ? Selector::AUTO : Selector::OFF;
}

inline const char *selector_name(Selector value) {
  switch (value) {
    case Selector::ON: return "On";
    case Selector::AUTO: return "Auto";
    case Selector::INVALID: return "Invalid";
    default: return "Off";
  }
}

struct Settings {
  uint32_t spin_duration_ms{30000};
  uint32_t system_duration_ms{3000};
  uint32_t away_duration_ms{300000};
  uint32_t spin_interval_ms{86400000};
  uint32_t system_interval_ms{7200000};
  uint32_t away_interval_ms{43200000};
  uint32_t away_spin_duration_ms{30000};
  uint32_t away_spin_interval_ms{86400000};
  uint32_t uv_min_off_ms{300000};
  bool spin_auto{true};
  bool system_auto{true};
  bool uv_enabled{true};
  bool away_auto{false};  // Enable after choosing/calibrating the exchange duration.
  bool away_spin_auto{false};
};

struct Inputs {
  Mode mode{Mode::SHUTDOWN};
  Selector pump{Selector::OFF};
  Selector tank{Selector::OFF};
  bool ready{false};
};

struct Outputs {
  bool pump{false};
  bool tank{false};
  bool uv{false};
  bool spin{false};
  bool system{false};
  bool uv_pending{false};
  bool tank_blocked{false};
  bool tank_pending{false};
};

struct Indicators {
  bool pump{false}, tank{false}, spin{false}, uv_system{false};
};

// Normal-operation indication only. Bench mode retains direct ownership of LEDs.
inline Indicators indicators(uint32_t now, const Inputs &in, const Outputs &out,
                             bool wifi_connected, bool spin_schedule_enabled = false,
                             bool uv_current_fault = false) {
  const bool slow = (now / 500) % 2 == 0;  // 1 Hz: blocked/waiting.
  const bool fast = (now / 250) % 2 == 0;  // 2 Hz: flushing/invalid contacts.
  Indicators led;
  led.pump = in.pump == Selector::INVALID ? fast : out.pump;
  led.tank = in.tank == Selector::INVALID ? fast : (out.tank_blocked || out.tank_pending) ? slow : out.tank;
  led.spin = out.spin ? fast : in.ready && in.mode != Mode::SHUTDOWN && spin_schedule_enabled;
  led.uv_system = out.system ? fast : out.uv_pending ? slow : out.uv;
  if (!wifi_connected && !out.spin && !out.system) {
    // Two 200 ms flashes, then a pause; repeat every five seconds.
    // This shared pattern takes priority over idle UV indication, never a flush.
    const uint32_t phase = now % 5000;
    led.spin = led.uv_system = phase < 200 || (phase >= 400 && phase < 600);
  }
  // A confirmed electrical fault outranks all other UV LED indications.
  if (uv_current_fault) led.uv_system = (now / 100) % 2 == 0; // Rapid: 5 Hz.
  return led;
}

inline bool remote_mode_allowed(Mode requested, const Inputs &in, bool bench_active) {
  return requested == Mode::SHUTDOWN || (!bench_active && in.ready &&
      in.pump == Selector::AUTO && in.tank == Selector::AUTO);
}

// Explicit bench ownership bypasses operating interlocks for individual wiring tests.
// A new session always starts empty and never restores outputs after reboot.
class BenchTest {
 public:
  void begin() { mask_ = 0; active_ = true; }
  void end() { mask_ = 0; active_ = false; }
  void all_off() { mask_ = 0; }
  bool active() const { return active_; }
  uint16_t mask() const { return active_ ? mask_ : 0; }
  bool set(unsigned channel, bool on) {
    if (!active_ || channel < 1 || channel > 16) return false;
    const uint16_t bit = uint16_t(1u << (channel - 1));
    mask_ = on ? uint16_t(mask_ | bit) : uint16_t(mask_ & ~bit);
    return true;
  }
 private:
  bool active_{false};
  uint16_t mask_{0};
};

// Positions describe permissions; deliberate selector movements request modes.
// Initial contact publication must not erase saved Away/Shutdown after reboot.
class LocalSelectors {
 public:
  bool request(const Inputs &in, Mode &mode) {
    if (!in.ready || in.pump == Selector::INVALID || in.tank == Selector::INVALID) return false;
    const bool resume = seen_ &&
        ((in.pump != pump_ && (in.pump == Selector::ON || in.pump == Selector::AUTO)) ||
         (in.tank != tank_ && (in.tank == Selector::ON || in.tank == Selector::AUTO)));
    pump_ = in.pump;
    tank_ = in.tank;
    seen_ = true;
    if (in.pump == Selector::OFF && in.tank == Selector::OFF) {
      mode = Mode::SHUTDOWN;
      return true;
    }
    if (resume) {
      mode = Mode::NORMAL;
      return true;
    }
    return false;
  }
 private:
  bool seen_{false};
  Selector pump_{Selector::OFF}, tank_{Selector::OFF};
};

// Away schedules use elapsed time. Enabling, changing frequency, entering Away,
// or rebooting starts a full interval. Duration edits never move a deadline.
class AwaySchedule {
 public:
  void update(uint32_t now, bool enabled, uint32_t interval, bool reset) {
    if (!initialized_ || reset || enabled != enabled_ || interval != interval_)
      since_ = now;
    initialized_ = true;
    enabled_ = enabled;
    interval_ = interval;
  }
  bool enabled() const { return initialized_ && enabled_ && interval_ > 0; }
  uint32_t remaining(uint32_t now) const {
    const uint32_t spent = now - since_;
    return spent < interval_ ? interval_ - spent : 0;
  }
  bool due(uint32_t now) const { return enabled() && remaining(now) == 0; }
  void started(uint32_t now) { since_ = now; }
 private:
  bool initialized_{false}, enabled_{false};
  uint32_t since_{0}, interval_{0};
};

class Controller {
 public:
  // The absolute per-run limit is independent of editable settings.
  static constexpr uint32_t MAX_FLUSH_MS = 300000;
  static constexpr uint32_t MAX_AWAY_FLUSH_MS = 1800000;
  static constexpr uint32_t PUMP_SETTLE_MS = 1000;
  // Stagger heater startup after each pump enable. This is not water detection.
  static constexpr uint32_t TANK_START_DELAY_MS = 5000;
  static constexpr uint32_t VALVE_GAP_MS = 1000;

  Outputs step(uint32_t now, const Inputs &in, const Settings &settings,
               Request request = Request::NONE) {
    const bool mode_changed = !started_ || in.mode != mode_;
    inputs_ = in;
    away_system_schedule_.update(now, settings.away_auto, settings.away_interval_ms, mode_changed);
    away_spin_schedule_.update(now, settings.away_spin_auto, settings.away_spin_interval_ms, mode_changed);
    if (!started_) {
      started_ = true;
      last_spin_ = last_system_ = last_closed_ = uv_off_since_ = now;
      mode_ = in.mode;
    }
    if (in.mode != mode_) {
      finish_flush(now);
      mode_ = in.mode;
      last_spin_ = last_system_ = now;
      // An event coincident with a mode change belongs to the old state.
      request = Request::NONE;
    }

    Outputs next;
    const bool permitted = in.ready && in.mode != Mode::SHUTDOWN;
    const bool pump_selected = in.pump == Selector::ON || in.pump == Selector::AUTO;
    const bool pump_base = permitted &&
        (in.pump == Selector::ON || (in.pump == Selector::AUTO &&
         (in.mode == Mode::NORMAL)));
    const bool manual_flush_permitted = permitted && pump_base;

    // Cancellation always precedes deciding whether a new run may start.
    if (active_ != Flush::NONE) {
      const bool away = away_flush();
      const bool same_button =
          (request == Request::SPIN && (active_ == Flush::SPIN || active_ == Flush::AWAY_SPIN)) ||
          (request == Request::SYSTEM && (active_ == Flush::SYSTEM || active_ == Flush::AWAY));
      const bool lost_permissive = !permitted || (away ?
          (!pump_selected || in.mode != Mode::AWAY) : !manual_flush_permitted);
      const bool done = opening_ ? elapsed(now, started_at_) >= duration_ms_
          : elapsed(now, started_at_) >= PUMP_SETTLE_MS + duration_ms_;
      if (lost_permissive || done || same_button || request == Request::CANCEL) {
        finish_flush(now);
        request = Request::NONE;
      }
    }

    const bool can_start = permitted && active_ == Flush::NONE &&
                          elapsed(now, last_closed_) >= VALVE_GAP_MS;
    if (can_start) {
      if (request == Request::AWAY && in.mode == Mode::AWAY && pump_selected) {
        start_flush(now, Flush::AWAY, settings.away_duration_ms);
      } else if (request == Request::SPIN && in.mode == Mode::AWAY && pump_selected) {
        start_flush(now, Flush::AWAY_SPIN, settings.away_spin_duration_ms);
      } else if (manual_flush_permitted && request == Request::SPIN) {
        start_flush(now, Flush::SPIN, settings.spin_duration_ms);
      } else if (manual_flush_permitted && request == Request::SYSTEM) {
        start_flush(now, Flush::SYSTEM, settings.system_duration_ms);
      } else if (request == Request::NONE && in.mode == Mode::AWAY &&
                 pump_selected) {
        if (away_spin_schedule_.due(now))
          start_flush(now, Flush::AWAY_SPIN, settings.away_spin_duration_ms);
        else if (away_system_schedule_.due(now))
          start_flush(now, Flush::AWAY, settings.away_duration_ms);
      } else if (request == Request::NONE && pump_base && in.mode == Mode::NORMAL) {
        if (settings.spin_auto && settings.spin_interval_ms > 0 &&
            elapsed(now, last_spin_) >= settings.spin_interval_ms) {
          start_flush(now, Flush::SPIN, settings.spin_duration_ms);
        } else if (settings.system_auto && settings.system_interval_ms > 0 &&
                   elapsed(now, last_system_) >= settings.system_interval_ms) {
          start_flush(now, Flush::SYSTEM, settings.system_duration_ms);
        }
      }
    }

    next.pump = pump_base || (permitted && pump_selected && away_flush());
    if (next.pump && !out_.pump) {
      pump_on_since_ = now;
      pump_starting_ = true;
    }
    // Latch completion until another pump enable; a millis rollover weeks later
    // must not briefly re-enter the heater startup delay.
    if (!next.pump || (pump_starting_ && elapsed(now, pump_on_since_) >= TANK_START_DELAY_MS))
      pump_starting_ = false;
    if (active_ != Flush::NONE && !opening_ && (next.pump && elapsed(now, pump_on_since_) >= PUMP_SETTLE_MS)) {
      opening_ = true;
      started_at_ = now;  // Begin the fixed valve-open deadline exactly once.
    }
    next.spin = opening_ && (active_ == Flush::SPIN || active_ == Flush::AWAY_SPIN);
    next.system = opening_ && (active_ == Flush::SYSTEM || active_ == Flush::AWAY);
    const bool want_tank = permitted && !away_flush() &&
        (in.tank == Selector::ON || (in.tank == Selector::AUTO &&
        (in.mode == Mode::NORMAL)));
    next.tank_pending = want_tank && next.pump && pump_starting_;
    next.tank = want_tank && next.pump && !next.tank_pending;
    next.tank_blocked = want_tank && !next.pump;

    // Candidate UV rule: continuous while using water, immediate off when
    // disabled, and at least five minutes off before a subsequent start.
    // There is no old pending request to replay after a mode/selector change.
    const bool want_uv = permitted && settings.uv_enabled && next.pump && !away_flush() &&
        (in.mode == Mode::NORMAL || (in.mode == Mode::AWAY && in.pump == Selector::ON));
    next.uv = want_uv && (out_.uv || elapsed(now, uv_off_since_) >= settings.uv_min_off_ms);
    next.uv_pending = want_uv && !next.uv;
    uv_min_off_ms_ = settings.uv_min_off_ms;
    if (out_.uv && !next.uv) uv_off_since_ = now;
    if (next.uv && !out_.uv) uv_on_since_ = now;
    out_ = next;
    return out_;
  }

  const Outputs &outputs() const { return out_; }
  Flush active_flush() const { return active_; }
  bool away_flush() const { return active_ == Flush::AWAY || active_ == Flush::AWAY_SPIN; }
  uint32_t away_next_ms(bool spin, uint32_t now) const {
    return (spin ? away_spin_schedule_ : away_system_schedule_).remaining(now);
  }
  const char *away_schedule_status(bool spin, uint32_t now) const {
    const auto &schedule = spin ? away_spin_schedule_ : away_system_schedule_;
    if (active_ == (spin ? Flush::AWAY_SPIN : Flush::AWAY))
      return opening_ ? "Flushing now" : "Starting pump";
    if (!schedule.enabled()) return "Disabled";
    if (inputs_.mode != Mode::AWAY) return "Paused: select Away";
    if (!inputs_.ready) return "Paused: control inputs";
    if (inputs_.pump == Selector::OFF) return "Paused: pump selector Off";
    if (inputs_.pump == Selector::INVALID) return "Paused: invalid pump selector";
    if (schedule.due(now))
      return active_ != Flush::NONE ? "Due: waiting for other flush" : "Due: valve pause";
    return "Scheduled";
  }
  bool opening() const { return opening_; }
  uint32_t uv_on_since() const { return uv_on_since_; }
  uint32_t uv_wait_remaining_ms(uint32_t now) const {
    if (!out_.uv_pending) return 0;
    const uint32_t spent = elapsed(now, uv_off_since_);
    return spent < uv_min_off_ms_ ? uv_min_off_ms_ - spent : 0;
  }
  uint32_t remaining_ms(uint32_t now) const {
    if (active_ == Flush::NONE) return 0;
    if (!opening_) return duration_ms_;
    const uint32_t spent = elapsed(now, started_at_);
    return spent < duration_ms_ ? duration_ms_ - spent : 0;
  }
  const char *flush_name() const {
    switch (active_) {
      case Flush::SPIN: return opening_ ? "Spin flush" : "Spin flush: starting pump";
      case Flush::SYSTEM: return opening_ ? "System flush" : "System flush: starting pump";
      case Flush::AWAY: return opening_ ? "Away flush" : "Away flush: starting pump";
      case Flush::AWAY_SPIN: return opening_ ? "Away spin flush" : "Away spin flush: starting pump";
      default: return "Idle";
    }
  }

 private:
  static uint32_t elapsed(uint32_t now, uint32_t since) { return now - since; }
  void start_flush(uint32_t now, Flush kind, uint32_t duration) {
    active_ = kind;
    opening_ = false;
    started_at_ = now;
    if (kind == Flush::AWAY) away_system_schedule_.started(now);
    if (kind == Flush::AWAY_SPIN) away_spin_schedule_.started(now);
    const uint32_t maximum = kind == Flush::AWAY ? MAX_AWAY_FLUSH_MS : MAX_FLUSH_MS;
    duration_ms_ = duration < 1000 ? 1000 : duration > maximum ? maximum : duration;
  }
  void finish_flush(uint32_t now) {
    if (active_ == Flush::NONE) return;
    if (active_ == Flush::SPIN || active_ == Flush::AWAY_SPIN) last_spin_ = now;
    else last_system_ = now;
    active_ = Flush::NONE;
    opening_ = false;
    last_closed_ = now;
  }
  bool started_{false};
  bool opening_{false};
  bool pump_starting_{false};
  Mode mode_{Mode::SHUTDOWN};
  Flush active_{Flush::NONE};
  Outputs out_{};
  Inputs inputs_{};
  AwaySchedule away_system_schedule_, away_spin_schedule_;
  uint32_t started_at_{0}, duration_ms_{0}, last_spin_{0}, last_system_{0};
  uint32_t last_closed_{0}, pump_on_since_{0}, uv_off_since_{0}, uv_on_since_{0};
  uint32_t uv_min_off_ms_{300000};
};

}  // namespace water_closet
