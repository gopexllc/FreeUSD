#include <cassert>
#include <string>

#include "freeusd/sdf/path.hpp"
#include "freeusd/usd/stage.hpp"
#include "freeusd/usdPhysics/collisionAPI.hpp"

#ifndef FREEUSD_TEST_FIXTURES_DIR
#error "FREEUSD_TEST_FIXTURES_DIR must be set by CMake for usd_physics_collision_test"
#endif

namespace {

std::string fixture(const char* name) {
  return std::string(FREEUSD_TEST_FIXTURES_DIR) + "/" + name;
}

}  // namespace

int main() {
  using freeusd::sdf::Path;
  using freeusd::usd::Stage;
  using freeusd::usdPhysics::CollisionAPI;

  std::string err;
  auto stage = Stage::OpenFromRootFile(fixture("parity_physics_collision.usda"),
                                       freeusd::usd::RootLayerSublayersPolicy::DepthFirst, &err);
  assert(stage && err.empty());

  const CollisionAPI collider = CollisionAPI::ReadFromPrim(stage, Path::FromString("/World/Collider"));
  assert(collider);
  assert(collider.IsCollisionAPI());

  bool enabled = true;
  assert(collider.GetCollisionEnabled(&enabled, 1.0));
  assert(!enabled);

  auto inherit_stage = Stage::OpenFromRootFile(fixture("parity_physics_collision_inherit.usda"),
                                               freeusd::usd::RootLayerSublayersPolicy::DepthFirst, &err);
  assert(inherit_stage && err.empty());
  const CollisionAPI inherited = CollisionAPI::ReadFromPrim(inherit_stage, Path::FromString("/World/Collider"));
  assert(inherited);
  assert(inherited.IsCollisionAPI());
  assert(inherited.GetCollisionEnabled(&enabled, 1.0));
  assert(!enabled);

  return 0;
}
