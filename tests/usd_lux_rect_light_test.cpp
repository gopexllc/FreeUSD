#include <cassert>
#include <cmath>
#include <string>

#include "freeusd/sdf/path.hpp"
#include "freeusd/usd/stage.hpp"
#include "freeusd/usdLux/rectLight.hpp"

#ifndef FREEUSD_TEST_FIXTURES_DIR
#error "FREEUSD_TEST_FIXTURES_DIR must be set by CMake for usd_lux_rect_light_test"
#endif

namespace {

std::string fixture(const char* name) {
  return std::string(FREEUSD_TEST_FIXTURES_DIR) + "/" + name;
}

}  // namespace

int main() {
  using freeusd::sdf::Path;
  using freeusd::usd::Stage;
  using freeusd::usdLux::RectLight;

  std::string err;
  auto stage = Stage::OpenFromRootFile(fixture("parity_lux_rect.usda"), freeusd::usd::RootLayerSublayersPolicy::None,
                                      &err);
  assert(stage && err.empty());

  RectLight light = RectLight::ReadFromPrim(stage, Path::FromString("/World/Panel"));
  assert(light);

  float intensity = 0.0f;
  assert(light.GetIntensity(&intensity, 1.0));
  assert(std::fabs(intensity - 1200.0f) < 1e-5f);

  freeusd::gf::Vec3f color{};
  assert(light.GetColor(&color, 1.0));
  assert(std::fabs(color.x() - 0.95f) < 1e-5f);
  assert(std::fabs(color.y() - 0.98f) < 1e-5f);
  assert(std::fabs(color.z() - 1.0f) < 1e-5f);

  float width = 0.0f;
  assert(light.GetWidth(&width, 1.0));
  assert(std::fabs(width - 2.0f) < 1e-5f);

  float height = 0.0f;
  assert(light.GetHeight(&height, 1.0));
  assert(std::fabs(height - 1.0f) < 1e-5f);

  return 0;
}
