#pragma once

#include <array>
#include <cmath>

namespace freeusd::gf {

/// Three floats (``GfVec3f`` / ``float3``-shaped role; clean-room).
struct Vec3f {
  float data[3]{0.0F, 0.0F, 0.0F};

  constexpr float x() const noexcept { return data[0]; }
  constexpr float y() const noexcept { return data[1]; }
  constexpr float z() const noexcept { return data[2]; }

  constexpr void set(float vx, float vy, float vz) noexcept {
    data[0] = vx;
    data[1] = vy;
    data[2] = vz;
  }

  constexpr std::array<float, 3> as_array() const noexcept { return {data[0], data[1], data[2]}; }

  static constexpr Vec3f Zero() noexcept { return Vec3f{}; }
};

constexpr bool operator==(const Vec3f& a, const Vec3f& b) noexcept {
  return a.data[0] == b.data[0] && a.data[1] == b.data[1] && a.data[2] == b.data[2];
}

}  // namespace freeusd::gf
