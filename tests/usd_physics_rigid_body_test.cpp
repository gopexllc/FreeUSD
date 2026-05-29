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

  auto inherit_stage = Stage::OpenFromRootFile(fixture("parity_physics_rigid_body_inherit.usda"),
                                               freeusd::usd::RootLayerSublayersPolicy::DepthFirst, &err);
  assert(inherit_stage && err.empty());
  const RigidBodyAPI inherited = RigidBodyAPI::ReadFromPrim(inherit_stage, Path::FromString("/World/Body"));
  assert(inherited);
  assert(inherited.IsRigidBodyAPI());
  assert(inherited.GetMass(&mass, 1.0));
  assert(mass == 4.0f);

  auto kinematic_stage = Stage::OpenFromRootFile(fixture("parity_physics_rigid_body_kinematic.usda"),
                                                  freeusd::usd::RootLayerSublayersPolicy::DepthFirst, &err);
  assert(kinematic_stage && err.empty());
  const RigidBodyAPI kinematic = RigidBodyAPI::ReadFromPrim(kinematic_stage, Path::FromString("/World/Body"));
  assert(kinematic && kinematic.IsRigidBodyAPI());
  bool kinematic_enabled = false;
  assert(kinematic.GetKinematicEnabled(&kinematic_enabled, 1.0));
  assert(kinematic_enabled);

  return 0;
}
