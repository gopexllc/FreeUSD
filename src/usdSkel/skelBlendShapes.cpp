#include "freeusd/usdSkel/skelBlendShapes.hpp"

#include <unordered_map>
#include <vector>

#include "freeusd/usdSkel/skelAnimation.hpp"
#include "freeusd/usdSkel/skelRoot.hpp"
#include "freeusd/usdSkel/tokens.hpp"
#include "freeusd/vt/value.hpp"

namespace freeusd::usdSkel {
namespace {

bool read_float_array(const freeusd::vt::Value& v, std::vector<float>* out) {
  if (!out) {
    return false;
  }
  if (v.GetFloatArray(out) && !out->empty()) {
    return true;
  }
  float f{};
  double d{};
  if (v.GetFloat(&f)) {
    out->assign(1, f);
    return true;
  }
  if (v.GetDouble(&d)) {
    out->assign(1, static_cast<float>(d));
    return true;
  }
  return false;
}

std::vector<std::string> read_token_strings(const freeusd::usd::Prim& prim, const freeusd::tf::Token& name,
                                            double time) {
  std::vector<std::string> out;
  if (!prim.IsValid()) {
    return out;
  }
  const freeusd::vt::Value v = prim.GetAttribute(name, time);
  std::vector<freeusd::tf::Token> toks;
  if (!v.GetTokenArray(&toks)) {
    freeusd::tf::Token single;
    if (v.GetToken(&single) && !single.IsEmpty()) {
      toks.push_back(single);
    }
  }
  out.reserve(toks.size());
  for (const freeusd::tf::Token& t : toks) {
    if (!t.IsEmpty()) {
      out.push_back(t.GetText());
    }
  }
  return out;
}

std::optional<freeusd::sdf::Path> animation_source_path(const freeusd::usd::Prim& geom) {
  const std::vector<freeusd::sdf::Path> targets = geom.GetRelationshipTargets(tokens::skel_animationSource());
  if (!targets.empty() && !targets[0].IsEmpty()) {
    return targets[0];
  }
  if (const freeusd::usd::Prim parent = geom.GetParent(); parent.IsValid()) {
    return SkelRoot(parent).GetAnimationSourcePath();
  }
  return std::nullopt;
}

bool remap_animation_weights(const freeusd::usd::Prim& geom, const std::vector<std::string>& geom_tokens, double time,
                             std::vector<float>* out) {
  if (!out || geom_tokens.empty()) {
    return false;
  }
  const std::optional<freeusd::sdf::Path> anim_path = animation_source_path(geom);
  if (!anim_path.has_value()) {
    return false;
  }
  const std::shared_ptr<const freeusd::usd::Stage> stage = geom.GetStage();
  if (!stage) {
    return false;
  }
  const SkelAnimation anim(stage->GetPrimAtPath(*anim_path));
  if (!anim) {
    return false;
  }
  const std::vector<std::string> anim_tokens = read_token_strings(anim.prim, tokens::blendShapes(), time);
  std::vector<float> anim_weights;
  if (!read_float_array(anim.prim.GetAttribute(tokens::blendShapeWeights(), time), &anim_weights)) {
    return false;
  }
  if (anim_tokens.size() != anim_weights.size()) {
    return false;
  }
  std::unordered_map<std::string, float> by_token;
  by_token.reserve(anim_tokens.size());
  for (std::size_t i = 0; i < anim_tokens.size(); ++i) {
    by_token[anim_tokens[i]] = anim_weights[i];
  }
  out->assign(geom_tokens.size(), 0.0f);
  for (std::size_t i = 0; i < geom_tokens.size(); ++i) {
    const auto it = by_token.find(geom_tokens[i]);
    if (it != by_token.end()) {
      (*out)[i] = it->second;
    }
  }
  return true;
}

}  // namespace

SkelBlendShapes SkelBlendShapes::ReadFromGeomPrim(const freeusd::usd::Prim& geom) {
  SkelBlendShapes binding{geom};
  if (!geom.IsValid()) {
    return binding;
  }
  binding.blend_shape_tokens = read_token_strings(geom, tokens::skel_blendShapes(), 1.0);
  binding.target_paths = geom.GetRelationshipTargets(tokens::skel_blendShapeTargets());
  if (binding.target_paths.size() < binding.blend_shape_tokens.size()) {
    binding.target_paths.resize(binding.blend_shape_tokens.size());
  }
  return binding;
}

bool SkelBlendShapes::GetWeights(std::vector<float>* out, double time) const {
  if (!out || !geom_prim.IsValid() || blend_shape_tokens.empty()) {
    return false;
  }
  if (read_float_array(geom_prim.GetAttribute(tokens::blendShapeWeights(), time), out)) {
    if (out->size() == blend_shape_tokens.size()) {
      return true;
    }
    if (out->size() > blend_shape_tokens.size()) {
      out->resize(blend_shape_tokens.size());
      return true;
    }
  }
  out->clear();
  return remap_animation_weights(geom_prim, blend_shape_tokens, time, out);
}

std::vector<BlendShape> SkelBlendShapes::ResolveTargets() const {
  std::vector<BlendShape> shapes;
  if (!geom_prim.IsValid()) {
    return shapes;
  }
  const std::shared_ptr<const freeusd::usd::Stage> stage = geom_prim.GetStage();
  if (!stage) {
    return shapes;
  }
  shapes.reserve(blend_shape_tokens.size());
  for (const freeusd::sdf::Path& path : target_paths) {
    if (path.IsEmpty()) {
      shapes.emplace_back();
      continue;
    }
    shapes.push_back(BlendShape(stage->GetPrimAtPath(path)));
  }
  if (shapes.size() < blend_shape_tokens.size()) {
    shapes.resize(blend_shape_tokens.size());
  }
  return shapes;
}

}  // namespace freeusd::usdSkel
