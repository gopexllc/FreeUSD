#pragma once

#include <array>
#include <cmath>
#include <cstring>

namespace freeusd::gf {

struct Vec3d;

/// Row-major storage ``m[row*4+col]``; composed for **row** homogeneous vectors ``[x y z 1] * M`` (same convention as common USD authoring docs).
struct Matrix4d {
  std::array<double, 16> m{};

  static Matrix4d Identity() noexcept {
    Matrix4d r{};
    r.m[0] = r.m[5] = r.m[10] = r.m[15] = 1.0;
    return r;
  }

  /// Non-uniform scale along X/Y/Z (diagonal).
  static Matrix4d Scale(double sx, double sy, double sz) noexcept {
    Matrix4d r{};
    r.m[0] = sx;
    r.m[5] = sy;
    r.m[10] = sz;
    r.m[15] = 1.0;
    return r;
  }

  static Matrix4d Translate(double tx, double ty, double tz) noexcept;
  static Matrix4d Translate(const Vec3d& t) noexcept;

  /// Matrix product ``C = A * B`` such that ``v * C == (v * A) * B`` for row vectors ``v``.
  static Matrix4d Multiply(const Matrix4d& a, const Matrix4d& b) noexcept;

  double* data() noexcept { return m.data(); }
  const double* data() const noexcept { return m.data(); }
};

inline bool operator==(const Matrix4d& a, const Matrix4d& b) noexcept { return a.m == b.m; }

}  // namespace freeusd::gf
