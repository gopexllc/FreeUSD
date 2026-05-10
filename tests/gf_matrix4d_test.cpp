#include <cmath>
#include <cstdlib>

#include "freeusd/gf/matrix4d.hpp"

namespace {

bool near(double a, double b) { return std::fabs(a - b) < 1e-9; }

/// Row-vector times matrix (matches ``GfMatrix4d::TransformDir`` indexing).
void transform_dir(const double v[3], const freeusd::gf::Matrix4d& m, double out[3]) {
  const double* p = m.data();
  out[0] = v[0] * p[0] + v[1] * p[4] + v[2] * p[8];
  out[1] = v[0] * p[1] + v[1] * p[5] + v[2] * p[9];
  out[2] = v[0] * p[2] + v[1] * p[6] + v[2] * p[10];
}

}  // namespace

int main() {
  using freeusd::gf::Matrix4d;

  const Matrix4d rx90 = Matrix4d::RotateDegreesX(90.0);
  double v[3] = {0.0, 1.0, 0.0};
  double r[3]{};
  transform_dir(v, rx90, r);
  if (!near(r[0], 0.0) || !near(r[1], 0.0) || !near(r[2], 1.0)) {
    return 1;
  }

  const Matrix4d rz90 = Matrix4d::RotateDegreesZ(90.0);
  double vx[3] = {1.0, 0.0, 0.0};
  transform_dir(vx, rz90, r);
  if (!near(r[0], 0.0) || !near(r[1], 1.0) || !near(r[2], 0.0)) {
    return 2;
  }

  const Matrix4d euler = Matrix4d::RotateDegreesXYZ(12.0, -34.0, 56.0);
  const Matrix4d piece = Matrix4d::Multiply(Matrix4d::Multiply(Matrix4d::RotateDegreesX(12.0), Matrix4d::RotateDegreesY(-34.0)),
                                            Matrix4d::RotateDegreesZ(56.0));
  if (!(euler == piece)) {
    return 3;
  }

  // ``rotateZYX`` uses ``vec3`` (angle-about-X, angle-about-Y, angle-about-Z); matrix is ``zRot(z) * yRot(y) * xRot(x)``.
  const Matrix4d zyx = Matrix4d::RotateDegreesZYX(5.0, 7.0, 11.0);
  const Matrix4d zyx_piece =
      Matrix4d::Multiply(Matrix4d::Multiply(Matrix4d::RotateDegreesZ(11.0), Matrix4d::RotateDegreesY(7.0)),
                         Matrix4d::RotateDegreesX(5.0));
  if (!(zyx == zyx_piece)) {
    return 4;
  }

  Matrix4d inv{};
  const Matrix4d t = Matrix4d::Translate(2.0, -3.0, 5.0);
  if (!t.GetInverse(&inv)) {
    return 5;
  }
  const Matrix4d prod = Matrix4d::Multiply(t, inv);
  for (std::size_t i = 0; i < 16; ++i) {
    const double e = (i % 5 == 0) ? 1.0 : 0.0;
    if (!near(prod.m[i], e)) {
      return 6;
    }
  }

  const Matrix4d rx = Matrix4d::RotateDegreesX(33.0);
  if (!rx.GetInverse(&inv)) {
    return 7;
  }
  const Matrix4d prod2 = Matrix4d::Multiply(rx, inv);
  for (std::size_t i = 0; i < 16; ++i) {
    const double e = (i % 5 == 0) ? 1.0 : 0.0;
    if (!near(prod2.m[i], e)) {
      return 8;
    }
  }

  const Matrix4d sh = Matrix4d::Shear(1.0, 0.0, 0.0);
  const double hv[4] = {0.0, 1.0, 0.0, 1.0};
  double ox = hv[0] * sh.m[0] + hv[1] * sh.m[1] + hv[2] * sh.m[2] + hv[3] * sh.m[3];
  double oy = hv[0] * sh.m[4] + hv[1] * sh.m[5] + hv[2] * sh.m[6] + hv[3] * sh.m[7];
  double oz = hv[0] * sh.m[8] + hv[1] * sh.m[9] + hv[2] * sh.m[10] + hv[3] * sh.m[11];
  if (!near(ox, 1.0) || !near(oy, 1.0) || !near(oz, 0.0)) {
    return 9;
  }

  return 0;
}
