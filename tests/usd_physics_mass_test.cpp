#include <cassert>
#include <cmath>
#include <string>

#include "freeusd/gf/vec3f.hpp"
#include "freeusd/sdf/path.hpp"
#include "freeusd/usd/stage.hpp"
#include "freeusd/usdPhysics/massAPI.hpp"

#ifndef FREEUSD_TEST_FIXTURES_DIR
#error "FREEUSD_TEST_FIXTURES_DIR must be set by CMake for usd_physics_mass_test"
#endif

namespace {

std::string fixture(const char* name) {
  return std::string(FREEUSD_TEST_FIXTURES_DIR) + "/" + name;
}

}  // namespace

int main() {
  using freeusd::gf::Vec3f;
  using freeusd::sdf::Path;
  using freeusd::usd::Stage;
  using freeusd::usdPhysics::MassAPI;

  std::string err;
  auto stage = Stage::OpenFromRootFile(fixture("parity_physics_mass.usda"),
                                       freeusd::usd::RootLayerSublayersPolicy::DepthFirst, &err);
  assert(stage && err.empty());

  const MassAPI prop = MassAPI::ReadFromPrim(stage, Path::FromString("/World/Prop"));
  assert(prop);
  assert(prop.IsMassAPI());

  float density = 0.0f;
  assert(prop.GetDensity(&density, 1.0));
  assert(std::fabs(density - 2.0f) < 1e-5f);

  Vec3f com{};
  assert(prop.GetCenterOfMass(&com, 1.0));
  assert(std::fabs(com.x()) < 1e-5f && std::fabs(com.y() - 0.5f) < 1e-5f && std::fabs(com.z()) < 1e-5f);

  return 0;
}
