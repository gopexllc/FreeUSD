#pragma once

#include <array>
#include <cmath>

namespace freeusd::gf {

struct Vec3d {
  double data[3]{0, 0, 0};

  constexpr double x() const noexcept { return data[0]; }
  constexpr double y() const noexcept { return data[1]; }
  constexpr double z() const noexcept { return data[2]; }

  constexpr void set(double vx, double vy, double vz) noexcept {
    data[0] = vx;
    data[1] = vy;
    data[2] = vz;
  }

  constexpr std::array<double, 3> as_array() const noexcept {
    return {data[0], data[1], data[2]};
  }

  static constexpr Vec3d Zero() noexcept { return Vec3d{}; }
};

constexpr bool operator==(const Vec3d& a, const Vec3d& b) noexcept {
  return a.data[0] == b.data[0] && a.data[1] == b.data[1] && a.data[2] == b.data[2];
}

}  // namespace freeusd::gf
