#include <cassert>
#include <cmath>
#include <vector>

#include "freeusd/gf/matrix4d.hpp"
#include "freeusd/usdSkel/skinning.hpp"

namespace {

bool near(double a, double b, double eps = 1e-9) { return std::fabs(a - b) <= eps; }

}  // namespace

int main() {
  using freeusd::gf::Matrix4d;
  using freeusd::usdSkel::ComputeSkinningMatrices;

  {
    const std::vector<Matrix4d> world{Matrix4d::Translate(0.0, 2.0, 0.0)};
    const std::vector<Matrix4d> bind{Matrix4d::Identity()};
    std::vector<Matrix4d> palette{};
    assert(ComputeSkinningMatrices(world, bind, &palette));
    assert(palette.size() == 1u);
    assert(near(palette[0].m[13], 2.0));
  }

  {
    const Matrix4d id = Matrix4d::Identity();
    const std::vector<Matrix4d> world{id};
    const std::vector<Matrix4d> bind{id};
    std::vector<Matrix4d> palette{};
    assert(ComputeSkinningMatrices(world, bind, &palette));
    assert(palette.size() == 1u);
    for (std::size_t i = 0; i < 16; ++i) {
      const double e = (i % 5 == 0) ? 1.0 : 0.0;
      assert(near(palette[0].m[i], e));
    }
  }

  {
    const std::vector<Matrix4d> world{Matrix4d::Identity(), Matrix4d::Identity()};
    const std::vector<Matrix4d> bind{Matrix4d::Identity()};
    std::vector<Matrix4d> palette{};
    assert(!ComputeSkinningMatrices(world, bind, &palette));
  }

  {
    std::vector<Matrix4d> palette{};
    assert(!ComputeSkinningMatrices({}, {}, &palette));
    assert(!ComputeSkinningMatrices({Matrix4d::Identity()}, {Matrix4d::Identity()}, nullptr));
  }

  return 0;
}
