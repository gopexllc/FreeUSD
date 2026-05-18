#include <cassert>
#include <cmath>
#include <string>

#include "freeusd/sdf/path.hpp"
#include "freeusd/usd/stage.hpp"
#include "freeusd/usdLux/sphereLight.hpp"

#ifndef FREEUSD_TEST_FIXTURES_DIR
#error "FREEUSD_TEST_FIXTURES_DIR must be set by CMake for usd_lux_sphere_light_test"
#endif

namespace {

std::string fixture(const char* name) {
  return std::string(FREEUSD_TEST_FIXTURES_DIR) + "/" + name;
}

}  // namespace

int main() {
  using freeusd::sdf::Path;
  using freeusd::usd::Stage;
  using freeusd::usdLux::SphereLight;

  std::string err;
  auto stage = Stage::OpenFromRootFile(fixture("parity_lux_sphere.usda"), freeusd::usd::RootLayerSublayersPolicy::None,
                                      &err);
  assert(stage && err.empty());

  SphereLight light = SphereLight::ReadFromPrim(stage, Path::FromString("/World/Bulb"));
  assert(light);

  float intensity = 0.0f;
  assert(light.GetIntensity(&intensity, 1.0));
  assert(std::fabs(intensity - 500.0f) < 1e-5f);

  freeusd::gf::Vec3f color{};
  assert(light.GetColor(&color, 1.0));
  assert(std::fabs(color.x() - 1.0f) < 1e-5f);
  assert(std::fabs(color.y() - 0.9f) < 1e-5f);
  assert(std::fabs(color.z() - 0.7f) < 1e-5f);

  float radius = 0.0f;
  assert(light.GetRadius(&radius, 1.0));
  assert(std::fabs(radius - 0.25f) < 1e-5f);

  return 0;
}
