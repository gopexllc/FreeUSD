#include "freeusd/usdSkel/skelBinding.hpp"

#include <vector>

#include "freeusd/usdSkel/tokens.hpp"
#include "freeusd/vt/value.hpp"

namespace freeusd::usdSkel {
namespace {

bool read_int32_array(const freeusd::vt::Value& v, std::vector<int>* out) {
  if (!out) {
    return false;
  }
  std::vector<std::int32_t> raw;
  if (v.GetInt32Array(&raw)) {
    out->assign(raw.begin(), raw.end());
    return !out->empty();
  }
  std::int32_t i32{};
  std::int64_t i64{};
  if (v.GetInt32(&i32)) {
    out->assign(1, static_cast<int>(i32));
    return true;
  }
  if (v.GetInt64(&i64)) {
    out->assign(1, static_cast<int>(i64));
    return true;
  }
  return false;
}

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

std::optional<freeusd::sdf::Path> path_from_relationship_or_token(const freeusd::usd::Prim& prim,
                                                                   const freeusd::tf::Token& name) {
  if (!prim.IsValid()) {
    return std::nullopt;
  }
  const std::vector<freeusd::sdf::Path> targets = prim.GetRelationshipTargets(name);
  if (!targets.empty() && !targets[0].IsEmpty()) {
    return targets[0];
  }
  const freeusd::vt::Value v = prim.GetAttribute(name, 1.0);
  freeusd::tf::Token tok;
  if (v.GetToken(&tok) && !tok.IsEmpty()) {
    const freeusd::sdf::Path p = freeusd::sdf::Path::FromString(tok.GetText());
    if (!p.IsEmpty()) {
      return p;
    }
  }
  return std::nullopt;
}

}  // namespace

std::optional<freeusd::sdf::Path> SkelBinding::ResolveSkeletonPath(const freeusd::usd::Prim& geom) {
  return path_from_relationship_or_token(geom, tokens::skel_skeleton());
}

bool SkelBinding::ReadJointIndices(const freeusd::usd::Prim& geom, std::vector<int>* out, double time) {
  if (!geom.IsValid() || !out) {
    return false;
  }
  return read_int32_array(geom.GetAttribute(tokens::primvars_skel_jointIndices(), time), out);
}

bool SkelBinding::ReadJointWeights(const freeusd::usd::Prim& geom, std::vector<float>* out, double time) {
  if (!geom.IsValid() || !out) {
    return false;
  }
  return read_float_array(geom.GetAttribute(tokens::primvars_skel_jointWeights(), time), out);
}

bool SkelBinding::ValidateInfluenceCounts(const std::vector<int>& indices, const std::vector<float>& weights,
                                          std::size_t influences_per_point) {
  if (indices.size() != weights.size()) {
    return false;
  }
  if (indices.empty()) {
    return false;
  }
  if (influences_per_point == 0) {
    return true;
  }
  return indices.size() % influences_per_point == 0;
}

SkelBinding SkelBinding::ReadFromGeomPrim(const freeusd::usd::Prim& geom, double time) {
  SkelBinding binding{geom};
  if (!geom.IsValid()) {
    return binding;
  }
  if (const std::optional<freeusd::sdf::Path> skel = ResolveSkeletonPath(geom)) {
    binding.skeleton_path = *skel;
  }
  (void)time;
  return binding;
}

}  // namespace freeusd::usdSkel
