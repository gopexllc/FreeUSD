#include <cassert>
#include <string>

#include "freeusd/sdf/path.hpp"
#include "freeusd/usd/stage.hpp"
#include "freeusd/usdPhysics/fixedJoint.hpp"

#ifndef FREEUSD_TEST_FIXTURES_DIR
#error "FREEUSD_TEST_FIXTURES_DIR must be set by CMake for usd_physics_fixed_joint_test"
#endif

namespace {

std::string fixture(const char* name) {
  return std::string(FREEUSD_TEST_FIXTURES_DIR) + "/" + name;
}

}  // namespace

int main() {
  using freeusd::sdf::Path;
  using freeusd::usd::Stage;
  using freeusd::usdPhysics::FixedJoint;

  std::string err;
  auto stage = Stage::OpenFromRootFile(fixture("parity_physics_fixed_joint.usda"),
                                       freeusd::usd::RootLayerSublayersPolicy::DepthFirst, &err);
  assert(stage && err.empty());

  const FixedJoint joint = FixedJoint::ReadFromPrim(stage, Path::FromString("/World/Anchor"));
  assert(joint);
  assert(joint.IsFixedJoint());

  Path body0;
  Path body1;
  assert(joint.GetBody0(&body0));
  assert(joint.GetBody1(&body1));
  assert(body0 == Path::FromString("/World/BodyA"));
  assert(body1 == Path::FromString("/World/BodyB"));

  bool enabled = false;
  assert(joint.GetJointEnabled(&enabled, 1.0));
  assert(enabled);

  return 0;
}
