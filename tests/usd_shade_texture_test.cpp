#include <cassert>
#include <string>

#include "freeusd/sdf/path.hpp"
#include "freeusd/usd/stage.hpp"
#include "freeusd/usdShade/material.hpp"
#include "freeusd/usdShade/previewSurface.hpp"

#ifndef FREEUSD_TEST_FIXTURES_DIR
#error "FREEUSD_TEST_FIXTURES_DIR must be set by CMake for usd_shade_texture_test"
#endif

namespace {

std::string fixture(const char* name) {
  return std::string(FREEUSD_TEST_FIXTURES_DIR) + "/" + name;
}

}  // namespace

int main() {
  using freeusd::sdf::Path;
  using freeusd::usd::Stage;
  using freeusd::usdShade::Material;
  using freeusd::usdShade::PreviewSurface;

  std::string err;
  auto stage = Stage::OpenFromRootFile(fixture("parity_shade_texture.usda"),
                                      freeusd::usd::RootLayerSublayersPolicy::None, &err);
  assert(stage && err.empty());

  const Material material = Material::ReadFromPrim(stage, Path::FromString("/World/Looks/Material"));
  assert(material);
  const Path shader_path = material.GetSurfaceShaderPath();
  assert(shader_path == Path::FromString("/World/Looks/Material/PreviewSurface"));

  const PreviewSurface preview = PreviewSurface::ReadFromPrim(stage, shader_path);
  assert(preview && preview.IsPreviewSurface());

  std::string texture_path;
  assert(preview.GetDiffuseTextureAssetPath(&texture_path, 1.0));
  assert(texture_path == "textures/albedo.png");

  return 0;
}
