#include <cassert>
#include <cmath>

#include "freeusd/gf/matrix4d.hpp"
#include "freeusd/gf/quatd.hpp"

int main() {
  using freeusd::gf::Matrix4d;
  using freeusd::gf::Quatd;
  const Quatd id = Quatd::Identity();
  assert(id.real == 1.0 && id.i == 0.0 && id.j == 0.0 && id.k == 0.0);

  Quatd q{0.0, 1.0, 0.0, 0.0};
  assert((q == Quatd{0.0, 1.0, 0.0, 0.0}));

  const Matrix4d m_id = Matrix4d::FromUnitQuaternion(id);
  assert(m_id == Matrix4d::Identity());

  const double s2 = std::sqrt(0.5);
  const Quatd qz90{s2, 0.0, 0.0, s2};
  const Matrix4d mq = Matrix4d::FromUnitQuaternion(qz90);
  const Matrix4d mz = Matrix4d::RotateDegreesZ(90.0);
  assert(mq == mz);

  return 0;
}
