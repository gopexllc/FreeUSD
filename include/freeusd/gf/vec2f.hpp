#pragma once

#include <array>
#include <cmath>

namespace freeusd::gf {

/// Two floats (``GfVec2f`` / ``float2``-shaped role; clean-room).
struct Vec2f {
  float data[2]{0.0F, 0.0F};

  constexpr float x() const noexcept { return data[0]; }
  constexpr float y() const noexcept { return data[1]; }

  constexpr void set(float vx, float vy) noexcept {
    data[0] = vx;
    data[1] = vy;
  }

  constexpr std::array<float, 2> as_array() const noexcept { return {data[0], data[1]}; }

  static constexpr Vec2f Zero() noexcept { return Vec2f{}; }
};

constexpr bool operator==(const Vec2f& a, const Vec2f& b) noexcept {
  return a.data[0] == b.data[0] && a.data[1] == b.data[1];
}

}  // namespace freeusd::gf
