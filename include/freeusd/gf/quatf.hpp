#pragma once

#include <cmath>

namespace freeusd::gf {

/// Unit quaternion with float components (``GfQuatf`` / ``quatf``-shaped role; clean-room).
/// Storage: **real** (**w**) plus **i**, **j**, **k** (vector part), matching ``(w, i, j, k)`` tuple order in USDA.
struct Quatf {
  float real{1.0F};
  float i{0.0F};
  float j{0.0F};
  float k{0.0F};

  constexpr Quatf() noexcept = default;
  constexpr Quatf(float w, float x, float y, float z) noexcept : real(w), i(x), j(y), k(z) {}

  static constexpr Quatf Identity() noexcept { return Quatf{}; }

  Quatf Normalized() const noexcept {
    const float n2 = real * real + i * i + j * j + k * k;
    if (!(n2 > 0.0F) || !std::isfinite(n2)) {
      return Identity();
    }
    const float inv = 1.0F / std::sqrt(n2);
    return Quatf{real * inv, i * inv, j * inv, k * inv};
  }
};

constexpr bool operator==(const Quatf& a, const Quatf& b) noexcept {
  return a.real == b.real && a.i == b.i && a.j == b.j && a.k == b.k;
}

constexpr bool operator!=(const Quatf& a, const Quatf& b) noexcept { return !(a == b); }

}  // namespace freeusd::gf
