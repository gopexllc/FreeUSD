#pragma once

namespace freeusd::gf {

/// Unit quaternion with double components (GfQuatd-shaped role; clean-room).
/// Storage: **`real`** (scalar) plus **`i`**, **`j`**, **`k`** (vector part).
struct Quatd {
  double real{1.0};
  double i{0.0};
  double j{0.0};
  double k{0.0};

  static constexpr Quatd Identity() noexcept { return Quatd{}; }
};

constexpr bool operator==(const Quatd& a, const Quatd& b) noexcept {
  return a.real == b.real && a.i == b.i && a.j == b.j && a.k == b.k;
}

constexpr bool operator!=(const Quatd& a, const Quatd& b) noexcept { return !(a == b); }

}  // namespace freeusd::gf
