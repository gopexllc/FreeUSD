#pragma once

#include <array>
#include <cstring>

namespace freeusd::gf {

/// Row-major 4x4 matrix (UsdGeom-style math container; semantics are host-defined).
struct Matrix4d {
  std::array<double, 16> m{};

  static Matrix4d Identity() noexcept {
    Matrix4d r{};
    r.m[0] = r.m[5] = r.m[10] = r.m[15] = 1.0;
    return r;
  }

  double* data() noexcept { return m.data(); }
  const double* data() const noexcept { return m.data(); }
};

inline bool operator==(const Matrix4d& a, const Matrix4d& b) noexcept { return a.m == b.m; }

}  // namespace freeusd::gf
