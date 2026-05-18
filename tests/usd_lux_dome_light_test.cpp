#include <cassert>
#include <cmath>
#include <string>

#include "freeusd/sdf/path.hpp"
#include "freeusd/usd/stage.hpp"
#include "freeusd/usdLux/domeLight.hpp"

#ifndef FREEUSD_TEST_FIXTURES_DIR
#error "FREEUSD_TEST_FIXTURES_DIR must be set by CMake for usd_lux_dome_light_test"
#endif

namespace {

std::string fixture(const char* name) {
  return std::string(FREEUSD_TEST_FIXTURES_DIR) + "/" + name;
}

}  // namespace

int main() {
  using freeusd::sdf::Path;
  using freeusd::usd::Stage;
  using freeusd::usdLux::DomeLight;

  std::string err;
  auto stage = Stage::OpenFromRootFile(fixture("parity_lux_dome.usda"), freeusd::usd::RootLayerSublayersPolicy::None,
                                      &err);
  assert(stage && err.empty());

  DomeLight light = DomeLight::ReadFromPrim(stage, Path::FromString("/World/Sky"));
  assert(light);

  float intensity = 0.0f;
  assert(light.GetIntensity(&intensity, 1.0));
  assert(std::fabs(intensity - 1.0f) < 1e-5f);

  std::string texture_path;
  assert(light.GetTextureFileAssetPath(&texture_path, 1.0));
  assert(texture_path == "textures/sky.hdr");

  std::string format;
  assert(light.GetTextureFormat(&format, 1.0));
  assert(format == "latlong");

  return 0;
}
