// glTF mapping exercised here (parity_skel_gltf.usda):
// Skeleton.joints              <-> skin.joints (ordered paths)
// bindTransforms             <-> inverse bind pose family
// SkelAnimation translations <-> channel.path "translation"
// SkelAnimation rotations    <-> channel.path "rotation"
// SkelAnimation scales       <-> channel.path "scale"

#include <cassert>
#include <cmath>
#include <optional>
#include <string>

#include "freeusd/sdf/path.hpp"
#include "freeusd/usd/stage.hpp"
#include "freeusd/usdSkel/gltfMapping.hpp"
#include "freeusd/usdSkel/skelAnimation.hpp"
#include "freeusd/usdSkel/skelBinding.hpp"
#include "freeusd/usdSkel/morphTargets.hpp"
#include "freeusd/usdSkel/skelRoot.hpp"
#include "freeusd/usdSkel/skinning.hpp"
#include "freeusd/usdSkel/skeleton.hpp"
#include "freeusd/usdGeom/tokens.hpp"
#include "freeusd/vt/value.hpp"

#ifndef FREEUSD_TEST_FIXTURES_DIR
#error "FREEUSD_TEST_FIXTURES_DIR must be set by CMake for usd_skel_smoke_test"
#endif

namespace {

std::string fixture(const char* name) {
  return std::string(FREEUSD_TEST_FIXTURES_DIR) + "/" + name;
}

bool near(double a, double b, double eps = 1e-5) {
  return std::fabs(a - b) <= eps;
}

}  // namespace

int main() {
  using freeusd::sdf::Path;
  using freeusd::usd::Stage;
  using freeusd::usdSkel::SkelAnimation;
  using freeusd::usdSkel::SkelBinding;
  using freeusd::usdSkel::SkelRoot;
  using freeusd::usdSkel::Skeleton;

  std::string err;
  auto stage = Stage::OpenFromRootFile(fixture("parity_skel_gltf.usda"), freeusd::usd::RootLayerSublayersPolicy::DepthFirst,
                                       &err);
  assert(stage && err.empty());

  const Path skel_path = Path::FromString("/World/Character/Skeleton");
  const Skeleton skel(stage->GetPrimAtPath(skel_path));
  assert(skel);

  const std::vector<std::string> joints = skel.GetJointNames();
  assert(joints.size() == 2u);
  assert(joints[0] == "Root");
  assert(joints[1] == "Root/Hip");

  const std::vector<int> parents = skel.GetJointParentIndices();
  assert(parents.size() == 2u);
  assert(parents[0] == -1);
  assert(parents[1] == 0);

  std::vector<freeusd::gf::Matrix4d> bind{};
  assert(skel.GetBindTransforms(&bind, 1.0));
  assert(bind.size() == 2u);
  assert(near(bind[1].m[13], 1.0));

  const std::vector<freeusd::gf::Matrix4d> world = skel.ComputeWorldBindMatrices(1.0);
  assert(world.size() == 2u);
  assert(near(world[1].m[13], 1.0));

  const Path anim_path = Path::FromString("/World/Character/Anim");
  const SkelAnimation anim(stage->GetPrimAtPath(anim_path));
  assert(anim);

  const auto trans_times = anim.ListTranslationSampleTimes();
  assert(trans_times.size() == 2u);
  assert(trans_times[0] == 0.0);
  assert(trans_times[1] == 1.0);

  std::vector<freeusd::gf::Vec3f> translations{};
  assert(anim.GetTranslations(&translations, 0.0));
  assert(translations.size() == 2u);
  assert(near(translations[1].y(), 1.0f));

  assert(anim.GetTranslations(&translations, 1.0));
  assert(near(translations[1].y(), 2.0f));

  std::vector<freeusd::gf::Quatf> rotations{};
  assert(anim.GetRotations(&rotations, 1.0));
  assert(rotations.size() == 2u);
  assert(near(rotations[0].real, 0.7071068f, 1e-4f));

  const std::vector<int> rebuilt = freeusd::usdSkel::BuildJointParentIndices(joints);
  assert(rebuilt == parents);

  const std::vector<freeusd::gf::Matrix4d> accumulated =
      freeusd::usdSkel::AccumulateWorldTransforms(parents, bind);
  assert(accumulated.size() == world.size());
  assert(near(accumulated[1].m[13], 1.0));

  freeusd::usdSkel::JointTransform hip_t0{};
  assert(SkelAnimation::SampleJointTransform(stage, anim_path, 1, 0.0, &hip_t0));
  assert(hip_t0.has_translation);
  assert(near(hip_t0.translation.y(), 1.0f));

  const Skeleton from_stage = Skeleton::ReadFromPrim(stage, skel_path);
  assert(from_stage);
  assert(from_stage.GetJointNames() == joints);

  {
    auto bind_stage = Stage::OpenFromRootFile(fixture("parity_skel_binding.usda"),
                                              freeusd::usd::RootLayerSublayersPolicy::DepthFirst, &err);
    assert(bind_stage && err.empty());

    const Path root_path = Path::FromString("/World/SkelCharacter");
    const SkelRoot skel_root(SkelRoot::ReadFromPrim(bind_stage, root_path));
    assert(skel_root);

    const std::optional<Path> found_skel = skel_root.FindSkeletonPath();
    assert(found_skel.has_value());
    assert(*found_skel == Path::FromString("/World/SkelCharacter/Skeleton"));

    const std::optional<Path> anim_path_opt = skel_root.GetAnimationSourcePath();
    assert(anim_path_opt.has_value());
    assert(*anim_path_opt == Path::FromString("/World/SkelCharacter/Anim"));

    const Skeleton root_skel = skel_root.GetSkeleton();
    assert(root_skel);
    assert(root_skel.GetJointNames() == joints);

    const SkelAnimation root_anim = skel_root.GetAnimationSource();
    assert(root_anim);

    const Path body_path = Path::FromString("/World/SkelCharacter/Body");
    const freeusd::usd::Prim body_prim = bind_stage->GetPrimAtPath(body_path);
    const SkelBinding binding = SkelBinding::ReadFromGeomPrim(body_prim);
    assert(binding);
    assert(binding.skeleton_path == *found_skel);

    const std::optional<Path> resolved = SkelBinding::ResolveSkeletonPath(body_prim);
    assert(resolved == *found_skel);

    std::vector<int> indices{};
    std::vector<float> weights{};
    assert(SkelBinding::ReadJointIndices(body_prim, &indices));
    assert(SkelBinding::ReadJointWeights(body_prim, &weights));
    assert(indices.size() == 8u);
    assert(weights.size() == 8u);
    assert(SkelBinding::ValidateInfluenceCounts(indices, weights, 4));
    assert(indices[0] == 0 && indices[1] == 1);
    assert(near(weights[0], 1.0f));
    assert(near(weights[4], 0.5f));
    assert(near(weights[5], 0.5f));
  }

  {
    auto morph_stage = Stage::OpenFromRootFile(fixture("parity_skel_blend_shapes.usda"),
                                               freeusd::usd::RootLayerSublayersPolicy::DepthFirst, &err);
    assert(morph_stage && err.empty());
    const freeusd::usd::Prim face = morph_stage->GetPrimAtPath(Path::FromString("/World/Face"));
    const freeusd::usdSkel::MorphTargets morphs = freeusd::usdSkel::MorphTargets::ReadFromGeomPrim(face);
    assert(morphs);
    std::vector<freeusd::gf::Vec3f> base{};
    assert(morphs.GetBasePoints(&base, 1.0));
    assert(base.size() == 2u);
    std::vector<float> weights{};
    assert(morphs.GetWeights(&weights, 1.0));
    assert(weights.size() == 2u && near(weights[0], 1.0f));
    const std::vector<freeusd::usdSkel::BlendShape> targets = morphs.LoadBlendShapes(morph_stage, 1.0);
    assert(targets.size() == 2u);
    std::vector<freeusd::gf::Vec3f> morphed{};
    assert(freeusd::usdSkel::ApplyMorphTargetsToPoints(base, targets, weights, &morphed, 1.0));
    assert(morphed[0].y() == 1.0f);
    assert(near(morphed[0].z(), 0.5f));
    assert(morphed[1].y() == 1.0f);
  }

  {
    // glTF LBS: point' = sum(weight * (point * inverseBind * jointWorld))
    auto skin_stage = Stage::OpenFromRootFile(fixture("parity_skel_skinning.usda"),
                                             freeusd::usd::RootLayerSublayersPolicy::DepthFirst, &err);
    assert(skin_stage && err.empty());

    const Path body_path = Path::FromString("/World/SkelCharacter/Body");
    const freeusd::usd::Prim body = skin_stage->GetPrimAtPath(body_path);
    const Skeleton skel(skin_stage->GetPrimAtPath(Path::FromString("/World/SkelCharacter/Skeleton")));
    const SkelAnimation anim(skin_stage->GetPrimAtPath(Path::FromString("/World/SkelCharacter/Anim")));
    assert(skel && anim);

    std::vector<freeusd::gf::Vec3f> points{};
    const freeusd::vt::Value pv = body.GetAttribute(freeusd::usdGeom::tokens::points(), 1.0);
    assert(pv.GetVec3fArray(&points));
    assert(points.size() == 1u);

    std::vector<int> indices{};
    std::vector<float> jweights{};
    assert(SkelBinding::ReadJointIndices(body, &indices, 1.0));
    assert(SkelBinding::ReadJointWeights(body, &jweights, 1.0));

    std::vector<freeusd::gf::Matrix4d> bind{};
    assert(skel.GetBindTransforms(&bind, 1.0));

    std::vector<freeusd::gf::Matrix4d> joint_world{};
    assert(freeusd::usdSkel::BuildJointWorldMatricesFromAnimation(skel, anim, 1.0, &joint_world));
    assert(joint_world.size() == 2u);

    std::vector<freeusd::gf::Vec3f> deformed{};
    assert(freeusd::usdSkel::DeformPointsWithSkeleton(points, indices, jweights, 1, joint_world, bind, nullptr,
                                                      &deformed));
    assert(deformed.size() == 1u);
    assert(deformed[0].y() > points[0].y());
  }

  return 0;
}
