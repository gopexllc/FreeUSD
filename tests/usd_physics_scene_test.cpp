#include <cassert>
#include <cmath>
#include <string>

#include "freeusd/sdf/path.hpp"
#include "freeusd/usd/stage.hpp"
#include "freeusd/usdPhysics/physicsScene.hpp"

#ifndef FREEUSD_TEST_FIXTURES_DIR
#error "FREEUSD_TEST_FIXTURES_DIR must be set by CMake for usd_physics_scene_test"
#endif

namespace {

std::string fixture(const char* name) {
  return std::string(FREEUSD_TEST_FIXTURES_DIR) + "/" + name;
}

bool near(float a, float b, float eps = 1e-5f) {
  return std::fabs(a - b) <= eps;
}

}  // namespace

int main() {
  using freeusd::sdf::Path;
  using freeusd::usd::Stage;
  using freeusd::usdPhysics::PhysicsScene;

  std::string err;
  auto stage = Stage::OpenFromRootFile(fixture("parity_physics_scene.usda"),
                                       freeusd::usd::RootLayerSublayersPolicy::DepthFirst, &err);
  assert(stage && err.empty());

  const PhysicsScene scene = PhysicsScene::ReadFromPrim(stage, Path::FromString("/World/Physics"));
  assert(scene);
  assert(scene.IsPhysicsScene());

  freeusd::gf::Vec3f gravity_dir{};
  assert(scene.GetGravityDirection(&gravity_dir, 1.0));
  assert(near(gravity_dir.x(), 0.0f) && near(gravity_dir.y(), 0.0f) && near(gravity_dir.z(), -1.0f));

  float gravity_mag = 0.0f;
  assert(scene.GetGravityMagnitude(&gravity_mag, 1.0) && near(gravity_mag, 981.0f));

  return 0;
}
