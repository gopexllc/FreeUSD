#include <cassert>
#include <string>

#include "freeusd/sdf/path.hpp"
#include "freeusd/usd/stage.hpp"
#include "freeusd/usdPhysics/rigidBodyAPI.hpp"

#ifndef FREEUSD_TEST_FIXTURES_DIR
#error "FREEUSD_TEST_FIXTURES_DIR must be set by CMake for usd_physics_rigid_body_test"
#endif

namespace {

std::string fixture(const char* name) {
  return std::string(FREEUSD_TEST_FIXTURES_DIR) + "/" + name;
}

}  // namespace

int main() {
  using freeusd::sdf::Path;
  using freeusd::usd::Stage;
  using freeusd::usdPhysics::RigidBodyAPI;

  std::string err;
  auto stage = Stage::OpenFromRootFile(fixture("parity_physics_rigid_body.usda"),
                                       freeusd::usd::RootLayerSublayersPolicy::DepthFirst, &err);
  assert(stage && err.empty());

  const RigidBodyAPI body = RigidBodyAPI::ReadFromPrim(stage, Path::FromString("/World/Body"));
  assert(body);
  assert(body.IsRigidBodyAPI());

  float mass = 0.0f;
  assert(body.GetMass(&mass, 1.0));
  assert(mass == 2.5f);

  return 0;
}
