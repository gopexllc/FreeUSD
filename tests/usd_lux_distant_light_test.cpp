#include <cassert>
#include <cmath>
#include <string>

#include "freeusd/sdf/path.hpp"
#include "freeusd/usd/stage.hpp"
#include "freeusd/usdLux/distantLight.hpp"

#ifndef FREEUSD_TEST_FIXTURES_DIR
#error "FREEUSD_TEST_FIXTURES_DIR must be set by CMake for usd_lux_distant_light_test"
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
  using freeusd::usdLux::DistantLight;

  std::string err;
  auto stage = Stage::OpenFromRootFile(fixture("parity_lux_distant.usda"),
                                       freeusd::usd::RootLayerSublayersPolicy::DepthFirst, &err);
  assert(stage && err.empty());

  const DistantLight sun = DistantLight::ReadFromPrim(stage, Path::FromString("/World/Sun"));
  assert(sun);

  float intensity = 0.0f;
  assert(sun.GetIntensity(&intensity, 1.0) && near(intensity, 1200.0f));

  freeusd::gf::Vec3f color{};
  assert(sun.GetColor(&color, 1.0));
  assert(near(color.x(), 1.0f) && near(color.y(), 0.95f) && near(color.z(), 0.8f));

  float angle = 0.0f;
  assert(sun.GetAngle(&angle, 1.0) && near(angle, 0.53f));

  return 0;
}
