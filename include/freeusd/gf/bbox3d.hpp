#pragma once

#include <algorithm>

#include "freeusd/gf/vec3d.hpp"

namespace freeusd::gf {

/// Axis-aligned box in **R³** (GfBBox3d-shaped role; clean-room).
struct BBox3d {
  Vec3d min{};
  Vec3d max{};

  /// True when any **`min` axis** is strictly greater than the matching **`max`** axis (invalid / empty box).
  bool IsEmpty() const noexcept {
    return min.x() > max.x() || min.y() > max.y() || min.z() > max.z();
  }

  /// Canonical empty box (`min` lexicographically above `max` on every axis).
  static BBox3d Empty() noexcept {
    BBox3d b;
    b.min.set(1.0, 1.0, 1.0);
    b.max.set(0.0, 0.0, 0.0);
    return b;
  }

  /// Tight box covering **`a`** and **`b`** (componentwise min / max of corners).
  static BBox3d FromMinMax(const Vec3d& a, const Vec3d& b) noexcept {
    BBox3d out;
    out.min.set(std::min(a.x(), b.x()), std::min(a.y(), b.y()), std::min(a.z(), b.z()));
    out.max.set(std::max(a.x(), b.x()), std::max(a.y(), b.y()), std::max(a.z(), b.z()));
    return out;
  }

  /// Union of two boxes; empty boxes are identity for the other operand.
  static BBox3d Union(const BBox3d& a, const BBox3d& b) noexcept {
    if (a.IsEmpty()) {
      return b;
    }
    if (b.IsEmpty()) {
      return a;
    }
    return FromMinMax(
        Vec3d{std::min(a.min.x(), b.min.x()), std::min(a.min.y(), b.min.y()), std::min(a.min.z(), b.min.z())},
        Vec3d{std::max(a.max.x(), b.max.x()), std::max(a.max.y(), b.max.y()), std::max(a.max.z(), b.max.z())});
  }
};

inline bool operator==(const BBox3d& u, const BBox3d& v) noexcept {
  return u.min == v.min && u.max == v.max;
}

inline bool operator!=(const BBox3d& u, const BBox3d& v) noexcept { return !(u == v); }

}  // namespace freeusd::gf
