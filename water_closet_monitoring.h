#pragma once

#include <cmath>
#include <cstdint>

namespace water_closet {

// Raw CT signals are AC volts. Calibration is measured amperes per signal volt.
// Missing, stale and uncalibrated data must never turn into a valid zero current.
struct CurrentSample {
  float signal{0};
  uint32_t sampled_at{0};
  bool seen{false};
  void record(uint32_t now, float value) {
    signal = value; sampled_at = now; seen = std::isfinite(value) && value >= 0;
  }
  float amps(uint32_t now, float calibration, bool sampling) const {
    if (!sampling || !seen || now - sampled_at > 15000 ||
        !std::isfinite(calibration) || calibration <= 0) return NAN;
    const float value = signal * calibration;
    return std::isfinite(value) ? value : NAN;
  }
};

// Estimates between 10-second CT acquisitions; short cycles can be missed.
// Only known off-to-on transitions count as starts. Counters are since reboot.
class UsageMonitor {
 public:
  void update(uint32_t now, float amps, float threshold) {
    const bool valid = std::isfinite(amps) && amps >= 0 &&
                       std::isfinite(threshold) && threshold > 0;
    const uint32_t dt = now - last_tick_;
    if (known_ && valid && running_ && dt <= 2000) runtime_ms_ += dt;
    const bool running = valid && (amps >= threshold || (known_ && running_ && amps >= threshold * 0.8f));
    if (valid && known_ && !running_ && running) ++starts_;
    known_ = valid; running_ = running; last_tick_ = now;
  }
  bool known() const { return known_; }
  bool running() const { return running_; }
  uint32_t starts() const { return starts_; }
  uint64_t runtime_seconds() const { return runtime_ms_ / 1000; }
 private:
  bool known_{false}, running_{false};
  uint32_t last_tick_{0}, starts_{0};
  uint64_t runtime_ms_{0};
};

class UVHealth {
 public:
  void update(uint32_t now, bool enabled, bool commanded, float amps,
              float minimum, uint32_t warmup, uint32_t low_delay) {
    if (commanded && !commanded_) on_since_ = now;
    commanded_ = commanded;
    fault_ = false;
    if (!enabled) { reset_low("Disabled"); return; }
    if (!std::isfinite(minimum) || minimum <= 0) { reset_low("Not calibrated"); return; }
    if (!commanded) { reset_low("Off"); return; }
    if (!std::isfinite(amps) || amps < 0) { reset_low("Waiting for current sample"); return; }
    if (now - on_since_ < warmup) { reset_low("Warming up"); return; }
    if (amps >= minimum) { reset_low("Current OK"); return; }
    if (!low_) { low_ = true; low_since_ = now; }
    fault_ = now - low_since_ >= low_delay;
    status_ = fault_ ? "Low current fault" : "Low current: confirming";
  }
  bool fault() const { return fault_; }
  const char *status() const { return status_; }
 private:
  void reset_low(const char *status) { low_ = false; status_ = status; }
  bool commanded_{false}, low_{false}, fault_{false};
  uint32_t on_since_{0}, low_since_{0};
  const char *status_{"Disabled"};
};

}  // namespace water_closet
