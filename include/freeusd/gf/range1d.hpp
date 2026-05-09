#pragma once

namespace freeusd::gf {

/// Closed interval on the real line (GfRange1d-shaped role; clean-room).
/// **IsEmpty** is true when **`min > max`**, matching the usual “invalid / empty range” convention.
struct Range1d {
  double min{0.0};
  double max{-1.0};

  constexpr bool IsEmpty() const noexcept { return min > max; }

  static constexpr Range1d UnitInterval() noexcept { return Range1d{0.0, 1.0}; }
};

constexpr bool operator==(const Range1d& a, const Range1d& b) noexcept {
  return a.min == b.min && a.max == b.max;
}

constexpr bool operator!=(const Range1d& a, const Range1d& b) noexcept { return !(a == b); }

}  // namespace freeusd::gf
