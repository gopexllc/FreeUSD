#include "freeusd/gf/matrix4d.hpp"

#include "freeusd/gf/vec3d.hpp"

namespace freeusd::gf {

Matrix4d Matrix4d::Translate(double tx, double ty, double tz) noexcept {
  Matrix4d r = Identity();
  r.m[12] = tx;
  r.m[13] = ty;
  r.m[14] = tz;
  return r;
}

Matrix4d Matrix4d::Translate(const Vec3d& t) noexcept { return Translate(t.x(), t.y(), t.z()); }

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

}  // namespace freeusd::gf
