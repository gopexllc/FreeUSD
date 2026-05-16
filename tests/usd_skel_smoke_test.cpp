// glTF mapping exercised here (parity_skel_gltf.usda):
// Skeleton.joints              <-> skin.joints (ordered paths)
// bindTransforms             <-> inverse bind pose family
// SkelAnimation translations <-> channel.path "translation"
// SkelAnimation rotations    <-> channel.path "rotation"
// SkelAnimation scales       <-> channel.path "scale"

#include <cassert>
#include <cmath>
#include <string>

#include "freeusd/sdf/path.hpp"
#include "freeusd/usd/stage.hpp"
#include "freeusd/usdSkel/gltfMapping.hpp"
#include "freeusd/usdSkel/skelAnimation.hpp"
#include "freeusd/usdSkel/skeleton.hpp"

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

  return 0;
}
