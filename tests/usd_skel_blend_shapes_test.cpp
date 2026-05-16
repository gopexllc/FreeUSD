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

  const Path face_path = Path::FromString("/World/FaceRig/Face");
  const freeusd::usd::Prim face = stage->GetPrimAtPath(face_path);
  assert(face.IsValid());

  const SkelBlendShapes binding = SkelBlendShapes::ReadFromGeomPrim(face);
  assert(binding);
  assert(binding.blend_shape_tokens.size() == 2u);
  assert(binding.blend_shape_tokens[0] == "smile");
  assert(binding.blend_shape_tokens[1] == "blink");
  assert(binding.target_paths.size() == 2u);
  assert(binding.target_paths[0] == Path::FromString("/World/FaceRig/Smile"));

  const BlendShape smile(stage->GetPrimAtPath(Path::FromString("/World/FaceRig/Smile")));
  assert(smile);
  std::vector<freeusd::gf::Vec3f> smile_offsets{};
  assert(smile.GetOffsets(&smile_offsets, 1.0));
  assert(smile_offsets.size() == 4u);
  assert(near(smile_offsets[0].y(), 0.5f));

  std::vector<float> weights{};
  assert(binding.GetWeights(&weights, 0.0));
  assert(weights.size() == 2u);
  assert(near(weights[0], 0.0f));
  assert(near(weights[1], 0.0f));

  assert(binding.GetWeights(&weights, 1.0));
  assert(near(weights[0], 1.0f));
  assert(near(weights[1], 0.0f));

  assert(binding.GetWeights(&weights, 2.0));
  assert(near(weights[0], 1.0f));
  assert(near(weights[1], 1.0f));

  const MorphTargets morph = MorphTargets::ReadFromGeomPrim(face);
  assert(morph);
  assert(morph.GetBlendShapeTokens() == binding.blend_shape_tokens);

  std::vector<freeusd::gf::Vec3f> morphed{};
  assert(morph.EvaluatePoints(&morphed, 0.0));
  assert(morphed.size() == 4u);
  assert(near(morphed[0].y(), 0.0f));

  assert(morph.EvaluatePoints(&morphed, 1.0));
  assert(near(morphed[0].y(), 0.5f));
  assert(near(morphed[0].x(), 0.0f));

  assert(morph.EvaluatePoints(&morphed, 2.0));
  assert(near(morphed[0].y(), 0.5f));
  assert(near(morphed[2].y(), 0.75f));

  {
    std::vector<float> anim_weights{};
    const freeusd::usd::Prim anim_prim = stage->GetPrimAtPath(Path::FromString("/World/FaceRig/Anim"));
    const SkelBlendShapes anim_only = SkelBlendShapes::ReadFromGeomPrim(face);
    (void)anim_only;
    const freeusd::vt::Value wv = anim_prim.GetAttribute(freeusd::usdSkel::tokens::blendShapeWeights(), 1.0);
    assert(wv.GetFloatArray(&anim_weights));
    assert(anim_weights.size() == 3u);
    assert(near(anim_weights[0], 0.5f));
  }

  return 0;
}
