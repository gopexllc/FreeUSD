#pragma once

#include <cmath>

namespace freeusd::gf {

/// Unit quaternion with double components (GfQuatd-shaped role; clean-room).
/// Storage: **`real`** (scalar / **w**) plus **`i`**, **`j`**, **`k`** (vector part **x,y,z**), matching common USD
/// ``quatd`` layout ``(real, i, j, k)``.
struct Quatd {
  double real{1.0};
  double i{0.0};
  double j{0.0};
  double k{0.0};

  constexpr Quatd() noexcept = default;
  constexpr Quatd(double w, double x, double y, double z) noexcept : real(w), i(x), j(y), k(z) {}

  static constexpr Quatd Identity() noexcept { return Quatd{}; }

  /// Euclidean normalization; returns **Identity** if the norm is zero / denormal.
  Quatd Normalized() const noexcept {
    const double n2 = real * real + i * i + j * j + k * k;
    if (!(n2 > 0.0) || !std::isfinite(n2)) {
      return Identity();
    }
    const double inv = 1.0 / std::sqrt(n2);
    return Quatd{real * inv, i * inv, j * inv, k * inv};
  }
};

constexpr bool operator==(const Quatd& a, const Quatd& b) noexcept {
  return a.real == b.real && a.i == b.i && a.j == b.j && a.k == b.k;
}

constexpr bool operator!=(const Quatd& a, const Quatd& b) noexcept { return !(a == b); }

}  // namespace freeusd::gf
