#pragma once

#include <array>
#include <cmath>

namespace freeusd::gf {

/// Two doubles (``GfVec2d`` / ``double2``-shaped role; clean-room).
struct Vec2d {
  double data[2]{0.0, 0.0};

  constexpr double x() const noexcept { return data[0]; }
  constexpr double y() const noexcept { return data[1]; }

  constexpr void set(double vx, double vy) noexcept {
    data[0] = vx;
    data[1] = vy;
  }

  constexpr std::array<double, 2> as_array() const noexcept { return {data[0], data[1]}; }

  static constexpr Vec2d Zero() noexcept { return Vec2d{}; }
};

constexpr bool operator==(const Vec2d& a, const Vec2d& b) noexcept {
  return a.data[0] == b.data[0] && a.data[1] == b.data[1];
}

}  // namespace freeusd::gf
