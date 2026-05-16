// glTF mapping (parity_skel_blend_shapes.usda):
// Mesh.points + BlendShape.offsets  -> morph target POSITION
// blendShapeWeights                 -> mesh.weights

#include <cassert>
#include <cmath>
#include <string>
#include <vector>

#include "freeusd/sdf/path.hpp"
#include "freeusd/usd/stage.hpp"
#include "freeusd/usdSkel/blendShape.hpp"
#include "freeusd/usdSkel/morphTargets.hpp"
#include "freeusd/usdSkel/skelBlendShapes.hpp"

#ifndef FREEUSD_TEST_FIXTURES_DIR
#error "FREEUSD_TEST_FIXTURES_DIR must be set by CMake for usd_skel_blend_shapes_test"
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
  using freeusd::usdSkel::BlendShape;
  using freeusd::usdSkel::MorphTargets;
  using freeusd::usdSkel::SkelBlendShapes;

  std::string err;
  auto stage = Stage::OpenFromRootFile(fixture("parity_skel_blend_shapes.usda"),
                                       freeusd::usd::RootLayerSublayersPolicy::DepthFirst, &err);
  assert(stage && err.empty());

  const freeusd::usd::Prim face = stage->GetPrimAtPath(Path::FromString("/World/Face"));
  const SkelBlendShapes binding = SkelBlendShapes::ReadFromGeomPrim(face);
  assert(binding);
  assert(binding.blend_shape_tokens.size() == 2u);
  assert(binding.blend_shape_tokens[0] == "Smile");
  assert(binding.blend_shape_tokens[1] == "Blink");

  const BlendShape smile(stage->GetPrimAtPath(Path::FromString("/World/BlendShapes/Smile")));
  assert(smile);
  std::vector<freeusd::gf::Vec3f> offsets{};
  assert(smile.GetOffsets(&offsets, 1.0));
  assert(offsets.size() == 2u);
  assert(near(offsets[0].y(), 1.0f));

  std::vector<float> weights{};
  assert(binding.GetWeights(&weights, 1.0));
  assert(weights.size() == 2u);
  assert(near(weights[0], 1.0f));
  assert(near(weights[1], 0.5f));

  const MorphTargets morph = MorphTargets::ReadFromGeomPrim(face);
  std::vector<freeusd::gf::Vec3f> morphed{};
  assert(morph.EvaluatePoints(&morphed, 1.0));
  assert(morphed.size() == 2u);
  assert(near(morphed[0].y(), 1.0f));
  assert(near(morphed[0].z(), 0.5f));
  assert(near(morphed[1].y(), 1.0f));

  return 0;
}
