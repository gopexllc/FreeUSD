#include <cassert>
#include <cmath>
#include <string>

#include "freeusd/sdf/path.hpp"
#include "freeusd/usd/stage.hpp"
#include "freeusd/usdLux/cylinderLight.hpp"

#ifndef FREEUSD_TEST_FIXTURES_DIR
#error "FREEUSD_TEST_FIXTURES_DIR must be set by CMake for usd_lux_cylinder_light_test"
#endif

namespace {

std::string fixture(const char* name) {
  return std::string(FREEUSD_TEST_FIXTURES_DIR) + "/" + name;
}

}  // namespace

int main() {
  using freeusd::sdf::Path;
  using freeusd::usd::Stage;
  using freeusd::usdLux::CylinderLight;

  std::string err;
  auto stage = Stage::OpenFromRootFile(fixture("parity_lux_cylinder.usda"),
                                      freeusd::usd::RootLayerSublayersPolicy::None, &err);
  assert(stage && err.empty());

  CylinderLight light = CylinderLight::ReadFromPrim(stage, Path::FromString("/World/Tube"));
  assert(light);

  float length = 0.0f;
  assert(light.GetLength(&length, 1.0));
  assert(std::fabs(length - 2.5f) < 1e-5f);

  float radius = 0.0f;
  assert(light.GetRadius(&radius, 1.0));
  assert(std::fabs(radius - 0.05f) < 1e-5f);

  return 0;
}
