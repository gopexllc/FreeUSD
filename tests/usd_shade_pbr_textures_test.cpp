#include <cassert>
#include <cmath>
#include <string>

#include "freeusd/sdf/path.hpp"
#include "freeusd/usd/stage.hpp"
#include "freeusd/usdShade/previewSurface.hpp"

#ifndef FREEUSD_TEST_FIXTURES_DIR
#error "FREEUSD_TEST_FIXTURES_DIR must be set by CMake for usd_shade_pbr_textures_test"
#endif

namespace {

std::string fixture(const char* name) {
  return std::string(FREEUSD_TEST_FIXTURES_DIR) + "/" + name;
}

}  // namespace

int main() {
  using freeusd::sdf::Path;
  using freeusd::usd::Stage;
  using freeusd::usdShade::PreviewSurface;

  std::string err;
  auto stage = Stage::OpenFromRootFile(fixture("parity_shade_pbr_textures.usda"),
                                      freeusd::usd::RootLayerSublayersPolicy::None, &err);
  assert(stage && err.empty());

  const PreviewSurface preview =
      PreviewSurface::ReadFromPrim(stage, Path::FromString("/World/Looks/Material/PreviewSurface"));
  assert(preview && preview.IsPreviewSurface());

  freeusd::gf::Vec3f emissive{};
  assert(preview.GetEmissiveColor(&emissive, 1.0));
  assert(std::fabs(emissive.x() - 0.1f) < 1e-5f);

  std::string path;
  assert(preview.GetNormalTextureAssetPath(&path, 1.0));
  assert(path == "textures/normal.png");
  assert(preview.GetOcclusionTextureAssetPath(&path, 1.0));
  assert(path == "textures/ao.png");
  assert(preview.GetMetallicTextureAssetPath(&path, 1.0));
  assert(path == "textures/metallic.png");
  assert(preview.GetRoughnessTextureAssetPath(&path, 1.0));
  assert(path == "textures/roughness.png");

  return 0;
}
