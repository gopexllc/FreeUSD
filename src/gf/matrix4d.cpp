#include "freeusd/gf/matrix4d.hpp"

#include <array>
#include <cmath>
#include <utility>

#include "freeusd/gf/quatd.hpp"
#include "freeusd/gf/vec3d.hpp"

namespace freeusd::gf {
namespace {

constexpr double kPi = 3.14159265358979323846264338327950288;
inline double deg_to_rad(double deg) noexcept { return deg * (kPi / 180.0); }

void mat3_mul(const std::array<double, 9>& a, const std::array<double, 9>& b, std::array<double, 9>* out) noexcept {
  for (int i = 0; i < 3; ++i) {
    for (int j = 0; j < 3; ++j) {
      double s = 0;
      for (int k = 0; k < 3; ++k) {
        s += a[static_cast<std::size_t>(i * 3 + k)] * b[static_cast<std::size_t>(k * 3 + j)];
      }
      (*out)[static_cast<std::size_t>(i * 3 + j)] = s;
    }
  }
}

/// Row-major 3×3 blocks matching ``GfMatrix3d`` × ``GfVec3d`` / ``GfMatrix4d::TransformDir`` (row vector on the left).
std::array<double, 9> mat3_rx(double rad) noexcept {
  const double c = std::cos(rad);
  const double s = std::sin(rad);
  return {1.0, 0.0, 0.0, 0.0, c, s, 0.0, -s, c};
}

std::array<double, 9> mat3_ry(double rad) noexcept {
  const double c = std::cos(rad);
  const double s = std::sin(rad);
  return {c, 0.0, -s, 0.0, 1.0, 0.0, s, 0.0, c};
}

std::array<double, 9> mat3_rz(double rad) noexcept {
  const double c = std::cos(rad);
  const double s = std::sin(rad);
  return {c, s, 0.0, -s, c, 0.0, 0.0, 0.0, 1.0};
}

Matrix4d from_mat3(const std::array<double, 9>& r) noexcept {
  Matrix4d m = Matrix4d::Identity();
  m.m[0] = r[0];
  m.m[1] = r[1];
  m.m[2] = r[2];
  m.m[4] = r[3];
  m.m[5] = r[4];
  m.m[6] = r[5];
  m.m[8] = r[6];
  m.m[9] = r[7];
  m.m[10] = r[8];
  return m;
}

using Mat3Compose = std::array<double, 9> (*)(const std::array<double, 9>&, const std::array<double, 9>&,
                                                const std::array<double, 9>&);

Matrix4d euler_packed(double ax_deg, double ay_deg, double az_deg, Mat3Compose compose) noexcept {
  const std::array<double, 9> x = mat3_rx(deg_to_rad(ax_deg));
  const std::array<double, 9> y = mat3_ry(deg_to_rad(ay_deg));
  const std::array<double, 9> z = mat3_rz(deg_to_rad(az_deg));
  return from_mat3(compose(x, y, z));
}

// OpenUSD ``UsdGeomXformOp::GetOpTransform`` 3-axis products (see ``usdGeom/xformOp.cpp``).
std::array<double, 9> compose_xyz(const std::array<double, 9>& x, const std::array<double, 9>& y,
                                  const std::array<double, 9>& z) noexcept {
  std::array<double, 9> t1{};
  std::array<double, 9> t2{};
  mat3_mul(x, y, &t1);
  mat3_mul(t1, z, &t2);
  return t2;
}
std::array<double, 9> compose_xzy(const std::array<double, 9>& x, const std::array<double, 9>& y,
                                  const std::array<double, 9>& z) noexcept {
  std::array<double, 9> t1{};
  std::array<double, 9> t2{};
  mat3_mul(x, z, &t1);
  mat3_mul(t1, y, &t2);
  return t2;
}
std::array<double, 9> compose_yxz(const std::array<double, 9>& x, const std::array<double, 9>& y,
                                  const std::array<double, 9>& z) noexcept {
  std::array<double, 9> t1{};
  std::array<double, 9> t2{};
  mat3_mul(y, x, &t1);
  mat3_mul(t1, z, &t2);
  return t2;
}
std::array<double, 9> compose_yzx(const std::array<double, 9>& x, const std::array<double, 9>& y,
                                  const std::array<double, 9>& z) noexcept {
  std::array<double, 9> t1{};
  std::array<double, 9> t2{};
  mat3_mul(y, z, &t1);
  mat3_mul(t1, x, &t2);
  return t2;
}
std::array<double, 9> compose_zxy(const std::array<double, 9>& x, const std::array<double, 9>& y,
                                  const std::array<double, 9>& z) noexcept {
  std::array<double, 9> t1{};
  std::array<double, 9> t2{};
  mat3_mul(z, x, &t1);
  mat3_mul(t1, y, &t2);
  return t2;
}
std::array<double, 9> compose_zyx(const std::array<double, 9>& x, const std::array<double, 9>& y,
                                  const std::array<double, 9>& z) noexcept {
  std::array<double, 9> t1{};
  std::array<double, 9> t2{};
  mat3_mul(z, y, &t1);
  mat3_mul(t1, x, &t2);
  return t2;
}

}  // namespace

Matrix4d Matrix4d::Translate(double tx, double ty, double tz) noexcept {
  Matrix4d r = Identity();
  r.m[12] = tx;
  r.m[13] = ty;
  r.m[14] = tz;
  return r;
}

Matrix4d Matrix4d::Translate(const Vec3d& t) noexcept { return Translate(t.x(), t.y(), t.z()); }

Matrix4d Matrix4d::Shear(double h_xy, double h_xz, double h_yz) noexcept {
  Matrix4d r = Identity();
  r.m[1] = h_xy;
  r.m[2] = h_xz;
  r.m[6] = h_yz;
  return r;
}

Matrix4d Matrix4d::Multiply(const Matrix4d& a, const Matrix4d& b) noexcept {
  Matrix4d c{};
  for (int i = 0; i < 4; ++i) {
    for (int k = 0; k < 4; ++k) {
      double s = 0;
      for (int j = 0; j < 4; ++j) {
        s += a.m[static_cast<std::size_t>(i * 4 + j)] * b.m[static_cast<std::size_t>(j * 4 + k)];
      }
      c.m[static_cast<std::size_t>(i * 4 + k)] = s;
    }
  }
  return c;
}

bool Matrix4d::GetInverse(Matrix4d* out) const noexcept {
  if (!out) {
    return false;
  }
  double aug[4][8]{};
  for (int i = 0; i < 4; ++i) {
    for (int j = 0; j < 4; ++j) {
      aug[i][j] = m[static_cast<std::size_t>(i * 4 + j)];
    }
    aug[i][4 + i] = 1.0;
  }
  constexpr double kEps = 1e-15;
  for (int col = 0; col < 4; ++col) {
    int pivot = col;
    double best = std::fabs(aug[col][col]);
    for (int r = col + 1; r < 4; ++r) {
      const double v = std::fabs(aug[r][col]);
      if (v > best) {
        best = v;
        pivot = r;
      }
    }
    if (best < kEps) {
      return false;
    }
    if (pivot != col) {
      for (int c = 0; c < 8; ++c) {
        std::swap(aug[col][c], aug[pivot][c]);
      }
    }
    const double div = aug[col][col];
    for (int c = 0; c < 8; ++c) {
      aug[col][c] /= div;
    }
    for (int r = 0; r < 4; ++r) {
      if (r == col) {
        continue;
      }
      const double f = aug[r][col];
      if (f == 0.0) {
        continue;
      }
      for (int c = 0; c < 8; ++c) {
        aug[r][c] -= f * aug[col][c];
      }
    }
  }
  for (int i = 0; i < 4; ++i) {
    for (int j = 0; j < 4; ++j) {
      out->m[static_cast<std::size_t>(i * 4 + j)] = aug[i][4 + j];
    }
  }
  return true;
}

Matrix4d Matrix4d::RotateDegreesX(double degrees) noexcept { return from_mat3(mat3_rx(deg_to_rad(degrees))); }

Matrix4d Matrix4d::RotateDegreesY(double degrees) noexcept { return from_mat3(mat3_ry(deg_to_rad(degrees))); }

Matrix4d Matrix4d::RotateDegreesZ(double degrees) noexcept { return from_mat3(mat3_rz(deg_to_rad(degrees))); }

Matrix4d Matrix4d::RotateDegreesXYZ(double ax, double ay, double az) noexcept {
  return euler_packed(ax, ay, az, compose_xyz);
}

Matrix4d Matrix4d::RotateDegreesXZY(double ax, double ay, double az) noexcept {
  return euler_packed(ax, ay, az, compose_xzy);
}

Matrix4d Matrix4d::RotateDegreesYXZ(double ax, double ay, double az) noexcept {
  return euler_packed(ax, ay, az, compose_yxz);
}

Matrix4d Matrix4d::RotateDegreesYZX(double ax, double ay, double az) noexcept {
  return euler_packed(ax, ay, az, compose_yzx);
}

Matrix4d Matrix4d::RotateDegreesZXY(double ax, double ay, double az) noexcept {
  return euler_packed(ax, ay, az, compose_zxy);
}

Matrix4d Matrix4d::RotateDegreesZYX(double ax, double ay, double az) noexcept {
  return euler_packed(ax, ay, az, compose_zyx);
}

Matrix4d Matrix4d::FromUnitQuaternion(const Quatd& q_in) noexcept {
  const Quatd q = q_in.Normalized();
  const double r = q.real;
  const double i0 = q.i;
  const double i1 = q.j;
  const double i2 = q.k;
  Matrix4d m = Identity();
  m.m[0] = 1.0 - 2.0 * (i1 * i1 + i2 * i2);
  m.m[1] = 2.0 * (i0 * i1 + i2 * r);
  m.m[2] = 2.0 * (i2 * i0 - i1 * r);
  m.m[4] = 2.0 * (i0 * i1 - i2 * r);
  m.m[5] = 1.0 - 2.0 * (i2 * i2 + i0 * i0);
  m.m[6] = 2.0 * (i1 * i2 + i0 * r);
  m.m[8] = 2.0 * (i2 * i0 + i1 * r);
  m.m[9] = 2.0 * (i1 * i2 - i0 * r);
  m.m[10] = 1.0 - 2.0 * (i1 * i1 + i0 * i0);
  return m;
}

}  // namespace freeusd::gf
