#include <cassert>

#include "freeusd/gf/bbox3d.hpp"
#include "freeusd/gf/vec3d.hpp"

int main() {
  using freeusd::gf::BBox3d;
  using freeusd::gf::Vec3d;

  assert(BBox3d::Empty().IsEmpty());

  Vec3d a;
  a.set(0.0, 0.0, 0.0);
  Vec3d b;
  b.set(1.0, 2.0, 3.0);
  const BBox3d box = BBox3d::FromMinMax(a, b);
  assert(!box.IsEmpty());
  assert(box.min.x() == 0.0 && box.max.z() == 3.0);

  return 0;
}
