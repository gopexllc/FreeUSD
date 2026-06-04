#include <cassert>
#include <cmath>
#include <string>

#include "freeusd/sdf/path.hpp"
#include "freeusd/usd/stage.hpp"
#include "freeusd/usdGeom/tokens.hpp"
#include "freeusd/usdSkel/morphTargets.hpp"
#include "freeusd/usdSkel/skelAnimation.hpp"
#include "freeusd/usdSkel/skelBinding.hpp"
#include "freeusd/usdSkel/skelRoot.hpp"
#include "freeusd/usdSkel/skeleton.hpp"
#include "freeusd/usdSkel/skinning.hpp"
#include "freeusd/vt/value.hpp"

#ifndef FREEUSD_TEST_FIXTURES_DIR
#error "FREEUSD_TEST_FIXTURES_DIR must be set by CMake for usd_skel_pipeline_test"
#endif

namespace {

std::string fixture(const char* name) {
  return std::string(FREEUSD_TEST_FIXTURES_DIR) + "/" + name;
}

bool near(float a, float b) {
  return std::fabs(a - b) < 1e-4f;
}

}  // namespace

int main() {
  using freeusd::sdf::Path;
  using freeusd::usd::Stage;
  using freeusd::usdSkel::MorphTargets;
  using freeusd::usdSkel::SkelAnimation;
  using freeusd::usdSkel::SkelBinding;
  using freeusd::usdSkel::SkelRoot;
  using freeusd::usdSkel::Skeleton;

  std::string err;
  auto stage = Stage::OpenFromRootFile(fixture("parity_skel_gltf_pipeline.usda"),
                                       freeusd::usd::RootLayerSublayersPolicy::DepthFirst, &err);
  assert(stage && err.empty());

  const Path root_path = Path::FromString("/World/Character");
  const SkelRoot root = SkelRoot::ReadFromPrim(stage, root_path);
  assert(root);
  assert(root.FindSkeletonPath().has_value());
  assert(root.GetAnimationSourcePath().has_value());

  const auto bound = root.FindBoundGeomPaths();
  assert(bound.size() == 1u);
  assert(bound[0] == Path::FromString("/World/Character/Body"));

  const freeusd::usd::Prim body = stage->GetPrimAtPath(bound[0]);
  const SkelBinding binding = SkelBinding::ReadFromGeomPrim(body);
  assert(binding);

  MorphTargets morphs = MorphTargets::ReadFromGeomPrim(body);
  assert(morphs);
  const auto tokens = morphs.GetBlendShapeTokens();
  assert(tokens.size() == 2u);
  assert(tokens[0] == "Smile" && tokens[1] == "Blink");

  std::vector<freeusd::gf::Vec3f> morphed{};
  assert(morphs.EvaluatePoints(&morphed, 1.0));
  assert(morphed.size() == 1u);
  assert(near(morphed[0].z(), 0.65f));

  std::vector<int> indices{};
  std::vector<float> weights{};
  assert(SkelBinding::ReadJointIndices(body, &indices, 1.0));
  assert(SkelBinding::ReadJointWeights(body, &weights, 1.0));

  const Skeleton skel = root.GetSkeleton();
  const SkelAnimation anim = root.GetAnimationSource();
  assert(skel && anim);

  std::vector<freeusd::gf::Matrix4d> bind{};
  assert(skel.GetBindTransforms(&bind, 1.0));
  std::vector<freeusd::gf::Matrix4d> joint_world{};
  assert(freeusd::usdSkel::BuildJointWorldMatricesFromAnimation(skel, anim, 1.0, &joint_world));

  std::vector<freeusd::gf::Vec3f> deformed{};
  assert(freeusd::usdSkel::DeformPointsWithSkeleton(morphed, indices, weights, 1, joint_world, bind, nullptr,
                                                    &deformed));
  assert(deformed.size() == 1u);
  assert(deformed[0].y() > morphed[0].y());

  return 0;
}
