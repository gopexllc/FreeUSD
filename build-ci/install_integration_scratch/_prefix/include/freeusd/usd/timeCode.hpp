#pragma once

#include <cstdint>
#include <ostream>

namespace freeusd::usd {

/// Authored time coordinate on a stage (UsdTimeCode-shaped role; clean-room).
/// Uses explicit representation instead of encoding sentinels in `double` payload bits.
class TimeCode {
 public:
  static TimeCode Default() noexcept { return TimeCode{Repr::Default, 0.0}; }
  /// Sentinel meaning “before all numeric samples” (common USD authoring convention).
  static TimeCode EarliestTime() noexcept { return TimeCode{Repr::Earliest, 0.0}; }
  static TimeCode FromDouble(double t) noexcept { return TimeCode{Repr::Numeric, t}; }

  bool IsDefault() const noexcept { return repr_ == Repr::Default; }
  bool IsEarliestTime() const noexcept { return repr_ == Repr::Earliest; }
  bool IsNumeric() const noexcept { return repr_ == Repr::Numeric; }

  /// Numeric time, or `0.0` when not `IsNumeric()` (see docs on each factory).
  double GetValue() const noexcept {
    return repr_ == Repr::Numeric ? value_ : 0.0;
  }

  bool operator==(const TimeCode& o) const noexcept {
    if (repr_ != o.repr_) {
      return false;
    }
    return repr_ != Repr::Numeric || value_ == o.value_;
  }
  bool operator!=(const TimeCode& o) const noexcept { return !(*this == o); }

 private:
  enum class Repr : std::uint8_t { Default, Earliest, Numeric };
  TimeCode(Repr r, double v) noexcept : repr_(r), value_(v) {}

  Repr repr_{Repr::Default};
  double value_{0.0};
};

inline std::ostream& operator<<(std::ostream& os, const TimeCode& t) {
  if (t.IsDefault()) {
    return os << "TimeCode(Default)";
  }
  if (t.IsEarliestTime()) {
    return os << "TimeCode(Earliest)";
  }
  return os << "TimeCode(" << t.GetValue() << ")";
}

}  // namespace freeusd::usd
