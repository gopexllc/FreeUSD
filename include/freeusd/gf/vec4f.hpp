#pragma once

#include <array>
#include <cmath>

namespace freeusd::gf {

/// Four floats (``GfVec4f`` / ``float4``-shaped role; clean-room).
struct Vec4f {
  float data[4]{0.0F, 0.0F, 0.0F, 0.0F};

  constexpr float x() const noexcept { return data[0]; }
  constexpr float y() const noexcept { return data[1]; }
  constexpr float z() const noexcept { return data[2]; }
  constexpr float w() const noexcept { return data[3]; }

  constexpr void set(float vx, float vy, float vz, float vw) noexcept {
    data[0] = vx;
    data[1] = vy;
    data[2] = vz;
    data[3] = vw;
  }

  constexpr std::array<float, 4> as_array() const noexcept {
    return {data[0], data[1], data[2], data[3]};
  }

  static constexpr Vec4f Zero() noexcept { return Vec4f{}; }
};

constexpr bool operator==(const Vec4f& a, const Vec4f& b) noexcept {
  return a.data[0] == b.data[0] && a.data[1] == b.data[1] && a.data[2] == b.data[2] &&
         a.data[3] == b.data[3];
}

}  // namespace freeusd::gf
