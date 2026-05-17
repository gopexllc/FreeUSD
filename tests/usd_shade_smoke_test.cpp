#include <cassert>
#include <cmath>
#include <string>

#include "freeusd/sdf/path.hpp"
#include "freeusd/usd/stage.hpp"
#include "freeusd/usdShade/material.hpp"
#include "freeusd/usdShade/previewSurface.hpp"
#include "freeusd/usdShade/shader.hpp"
#include "freeusd/usdShade/tokens.hpp"

#ifndef FREEUSD_TEST_FIXTURES_DIR
#error "FREEUSD_TEST_FIXTURES_DIR must be set by CMake for usd_shade_smoke_test"
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
  using freeusd::usdShade::Material;
  using freeusd::usdShade::PreviewSurface;
  using freeusd::usdShade::Shader;

  std::string err;
  auto stage = Stage::OpenFromRootFile(fixture("parity_shade_preview.usda"),
                                       freeusd::usd::RootLayerSublayersPolicy::DepthFirst, &err);
  assert(stage && err.empty());

  const Path mat_path = Path::FromString("/World/Looks/Material");
  const Material mat = Material::ReadFromPrim(stage, mat_path);
  assert(mat);

  const Path surface_shader = mat.GetSurfaceShaderPath();
  assert(surface_shader == Path::FromString("/World/Looks/Material/PreviewSurface"));

  const std::vector<Path> shaders = mat.ListShaderPrimPaths();
  assert(shaders.size() == 1u);
  assert(shaders[0] == surface_shader);

  const Shader shader = Shader::ReadFromPrim(stage, surface_shader);
  assert(shader);
  assert(shader.GetShaderId().GetText() == std::string("UsdPreviewSurface"));

  freeusd::gf::Vec3f diffuse{};
  assert(shader.GetDiffuseColor(&diffuse, 1.0));
  assert(near(diffuse.x(), 0.8f) && near(diffuse.y(), 0.2f) && near(diffuse.z(), 0.1f));

  float metallic = 0.0f;
  float roughness = 0.0f;
  float opacity = 0.0f;
  assert(shader.GetMetallic(&metallic, 1.0) && near(metallic, 0.5f));
  assert(shader.GetRoughness(&roughness, 1.0) && near(roughness, 0.3f));
  assert(shader.GetOpacity(&opacity, 1.0) && near(opacity, 1.0f));

  const PreviewSurface preview = PreviewSurface::ReadFromPrim(stage, surface_shader);
  assert(preview);
  assert(preview.IsPreviewSurface());

  freeusd::gf::Vec3f diffuse2{};
  assert(preview.GetDiffuseColor(&diffuse2, 1.0));
  assert(near(diffuse2.x(), 0.8f));

  const freeusd::usd::Prim mesh = stage->GetPrimAtPath(Path::FromString("/World/Cube"));
  assert(mesh.IsValid());
  const std::vector<Path> bindings = mesh.GetRelationshipTargets(freeusd::tf::Token("material:binding"));
  assert(bindings.size() == 1u);
  assert(bindings[0] == mat_path);

  return 0;
}
