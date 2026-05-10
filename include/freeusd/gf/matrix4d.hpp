#pragma once

#include <array>
#include <cmath>
#include <cstring>

namespace freeusd::gf {

struct Quatd;
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

  /// Upper-triangular shear encoded by ``UsdGeomXformOp::TypeShear``-shaped ``double3`` \((h_{xy}, h_{xz}, h_{yz})\):
  /// \(x' = x + h_{xy} y + h_{xz} z\), \(y' = y + h_{yz} z\), \(z' = z\) for row vectors ``[x y z 1] * M``.
  static Matrix4d Shear(double h_xy, double h_xz, double h_yz) noexcept;

  /// Right-handed rotations about a fixed parent axis; angles in **degrees** (matches ``UsdGeomXformOp`` scalars).
  static Matrix4d RotateDegreesX(double degrees) noexcept;
  static Matrix4d RotateDegreesY(double degrees) noexcept;
  static Matrix4d RotateDegreesZ(double degrees) noexcept;

  /// Packed Euler triples (degrees). Arguments are always **rotation about X**, **about Y**, **about Z** (same as a
  /// ``double3`` / ``vec3d`` attribute), composed like OpenUSD ``UsdGeomXformOp::GetOpTransform`` (for example
  /// ``RotateDegreesZYX(ax,ay,az)`` uses ``zRot(az) * yRot(ay) * xRot(ax)``).
  static Matrix4d RotateDegreesXYZ(double ax, double ay, double az) noexcept;
  static Matrix4d RotateDegreesXZY(double ax, double ay, double az) noexcept;
  static Matrix4d RotateDegreesYXZ(double ax, double ay, double az) noexcept;
  static Matrix4d RotateDegreesYZX(double ax, double ay, double az) noexcept;
  static Matrix4d RotateDegreesZXY(double ax, double ay, double az) noexcept;
  static Matrix4d RotateDegreesZYX(double ax, double ay, double az) noexcept;

  /// Matrix product ``C = A * B`` such that ``v * C == (v * A) * B`` for row vectors ``v``.
  static Matrix4d Multiply(const Matrix4d& a, const Matrix4d& b) noexcept;

  /// Inverse ``B`` with ``Multiply(*this, B) == Identity()`` when **this** is non-singular (Gauss--Jordan; partial pivot).
  bool GetInverse(Matrix4d* out) const noexcept;

  /// Pure rotation from a (possibly non-unit) quaternion; internally **normalizes** (same role as
  /// ``GfMatrix4d(GfRotation(GfQuatd(...)), GfVec3d(0))`` / ``UsdGeomXformOp::TypeOrient``).
  static Matrix4d FromUnitQuaternion(const Quatd& q) noexcept;

  double* data() noexcept { return m.data(); }
  const double* data() const noexcept { return m.data(); }
};

inline bool operator==(const Matrix4d& a, const Matrix4d& b) noexcept { return a.m == b.m; }

}  // namespace freeusd::gf
