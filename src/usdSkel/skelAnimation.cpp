#include "freeusd/usdSkel/skelAnimation.hpp"

#include <string>
#include <vector>

#include "freeusd/usd/stage.hpp"
#include "freeusd/usdSkel/tokens.hpp"
#include "freeusd/vt/value.hpp"

namespace freeusd::usdSkel {
namespace {

std::vector<std::string> read_joint_names(const freeusd::usd::Prim& prim) {
  std::vector<std::string> names;
  if (!prim.IsValid()) {
    return names;
  }
  const freeusd::vt::Value v = prim.GetAttribute(tokens::joints(), 1.0);
  std::vector<freeusd::tf::Token> toks;
  if (!v.GetTokenArray(&toks)) {
    return names;
  }
  names.reserve(toks.size());
  for (const freeusd::tf::Token& t : toks) {
    if (!t.IsEmpty()) {
      names.push_back(t.GetText());
    }
  }
  return names;
}

bool read_vec3f_array_attr(const freeusd::usd::Prim& prim, const freeusd::tf::Token& name, double time,
                           std::vector<freeusd::gf::Vec3f>* out) {
  if (!out || !prim.IsValid()) {
    return false;
  }
  const freeusd::vt::Value v = prim.GetAttribute(name, time);
  return v.GetVec3fArray(out);
}

bool read_quatf_array_attr(const freeusd::usd::Prim& prim, const freeusd::tf::Token& name, double time,
                           std::vector<freeusd::gf::Quatf>* out) {
  if (!out || !prim.IsValid()) {
    return false;
  }
  const freeusd::vt::Value v = prim.GetAttribute(name, time);
  return v.GetQuatfArray(out);
}

}  // namespace

std::vector<std::string> SkelAnimation::GetJointNames() const { return read_joint_names(prim); }

bool SkelAnimation::GetTranslations(std::vector<freeusd::gf::Vec3f>* out, double time) const {
  return read_vec3f_array_attr(prim, tokens::translations(), time, out);
}

bool SkelAnimation::GetRotations(std::vector<freeusd::gf::Quatf>* out, double time) const {
  return read_quatf_array_attr(prim, tokens::rotations(), time, out);
}

bool SkelAnimation::GetScales(std::vector<freeusd::gf::Vec3f>* out, double time) const {
  return read_vec3f_array_attr(prim, tokens::scales(), time, out);
}

std::vector<double> SkelAnimation::ListTranslationSampleTimes() const {
  if (!prim.IsValid()) {
    return {};
  }
  return prim.ListAttributeSampleTimes(tokens::translations());
}

std::vector<double> SkelAnimation::ListRotationSampleTimes() const {
  if (!prim.IsValid()) {
    return {};
  }
  return prim.ListAttributeSampleTimes(tokens::rotations());
}

std::vector<double> SkelAnimation::ListScaleSampleTimes() const {
  if (!prim.IsValid()) {
    return {};
  }
  return prim.ListAttributeSampleTimes(tokens::scales());
}

bool SkelAnimation::SampleJointTransform(const std::shared_ptr<const freeusd::usd::Stage>& stage,
                                         const freeusd::sdf::Path& anim_path, std::size_t joint_index, double time,
                                         JointTransform* out) {
  if (!out || !stage) {
    return false;
  }
  const SkelAnimation anim(stage->GetPrimAtPath(anim_path));
  if (!anim) {
    return false;
  }

  JointTransform sample{};
  std::vector<freeusd::gf::Vec3f> translations;
  if (anim.GetTranslations(&translations, time) && joint_index < translations.size()) {
    sample.translation = translations[joint_index];
    sample.has_translation = true;
  }
  std::vector<freeusd::gf::Quatf> rotations;
  if (anim.GetRotations(&rotations, time) && joint_index < rotations.size()) {
    sample.rotation = rotations[joint_index];
    sample.has_rotation = true;
  }
  std::vector<freeusd::gf::Vec3f> scales;
  if (anim.GetScales(&scales, time) && joint_index < scales.size()) {
    sample.scale = scales[joint_index];
    sample.has_scale = true;
  }

  if (!sample.has_translation && !sample.has_rotation && !sample.has_scale) {
    return false;
  }
  *out = sample;
  return true;
}

}  // namespace freeusd::usdSkel
