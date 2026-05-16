#include "freeusd/usdSkel/morphTargets.hpp"

#include <algorithm>
#include <vector>

#include "freeusd/usd/stage.hpp"
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
  if (v.GetTokenArray(&toks)) {
    out.reserve(toks.size());
    for (const freeusd::tf::Token& t : toks) {
      if (!t.IsEmpty()) {
        out.push_back(t.GetText());
      }
    }
    return out;
  }
  freeusd::tf::Token single;
  if (v.GetToken(&single) && !single.IsEmpty()) {
    out.push_back(single.GetText());
  }
  return out;
}

void accumulate_blend_shape(const std::vector<freeusd::gf::Vec3f>& offsets, const std::vector<int>& point_indices,
                            float weight, std::vector<freeusd::gf::Vec3f>* points) {
  if (!points || weight == 0.0F || offsets.empty()) {
    return;
  }
  if (point_indices.empty()) {
    const std::size_t n = std::min(points->size(), offsets.size());
    for (std::size_t i = 0; i < n; ++i) {
      (*points)[i].set((*points)[i].x() + weight * offsets[i].x(), (*points)[i].y() + weight * offsets[i].y(),
                       (*points)[i].z() + weight * offsets[i].z());
    }
    return;
  }
  const std::size_t n = std::min(point_indices.size(), offsets.size());
  for (std::size_t k = 0; k < n; ++k) {
    const int idx = point_indices[k];
    if (idx < 0 || static_cast<std::size_t>(idx) >= points->size()) {
      continue;
    }
    (*points)[static_cast<std::size_t>(idx)].set(
        (*points)[static_cast<std::size_t>(idx)].x() + weight * offsets[k].x(),
        (*points)[static_cast<std::size_t>(idx)].y() + weight * offsets[k].y(),
        (*points)[static_cast<std::size_t>(idx)].z() + weight * offsets[k].z());
  }
}

}  // namespace

MorphTargets MorphTargets::ReadFromGeomPrim(const freeusd::usd::Prim& geom) { return MorphTargets{geom}; }

std::vector<std::string> MorphTargets::GetBlendShapeTokens() const {
  return read_token_strings(geom_prim, tokens::skel_blendShapes(), 1.0);
}

std::vector<freeusd::sdf::Path> MorphTargets::GetBlendShapeTargetPaths() const {
  if (!geom_prim.IsValid()) {
    return {};
  }
  return geom_prim.GetRelationshipTargets(tokens::skel_blendShapeTargets());
}

bool MorphTargets::GetWeights(std::vector<float>* out, double time) const {
  if (!out || !geom_prim.IsValid()) {
    return false;
  }
  return read_float_array(geom_prim.GetAttribute(tokens::blendShapeWeights(), time), out);
}

std::vector<MorphTargetBinding> MorphTargets::ResolveBindings(const std::shared_ptr<const freeusd::usd::Stage>& stage,
                                                              double time) const {
  (void)stage;
  std::vector<MorphTargetBinding> bindings;
  if (!geom_prim.IsValid()) {
    return bindings;
  }
  const std::vector<std::string> tokens_list = GetBlendShapeTokens();
  const std::vector<freeusd::sdf::Path> targets = GetBlendShapeTargetPaths();
  std::vector<float> weights;
  (void)GetWeights(&weights, time);

  const std::size_t n = std::max(tokens_list.size(), targets.size());
  bindings.reserve(n);
  for (std::size_t i = 0; i < n; ++i) {
    MorphTargetBinding entry{};
    if (i < tokens_list.size()) {
      entry.token = tokens_list[i];
    }
    if (i < targets.size()) {
      entry.target_path = targets[i];
    }
    if (i < weights.size()) {
      entry.weight = weights[i];
    }
    if (!entry.target_path.IsEmpty()) {
      bindings.push_back(std::move(entry));
    }
  }
  return bindings;
}

std::vector<BlendShape> MorphTargets::LoadBlendShapes(const std::shared_ptr<const freeusd::usd::Stage>& stage,
                                                      double time) const {
  (void)time;
  std::vector<BlendShape> shapes;
  if (!stage) {
    return shapes;
  }
  for (const MorphTargetBinding& binding : ResolveBindings(stage, 1.0)) {
    if (!binding.target_path.IsEmpty()) {
      BlendShape shape = BlendShape::ReadFromPrim(stage, binding.target_path);
      if (shape) {
        shapes.push_back(std::move(shape));
      }
    }
  }
  return shapes;
}

bool MorphTargets::ApplyToPoints(const std::shared_ptr<const freeusd::usd::Stage>& stage,
                                 const std::vector<freeusd::gf::Vec3f>& base_points,
                                 std::vector<freeusd::gf::Vec3f>* out, double time) const {
  if (!stage || !out) {
    return false;
  }
  const std::vector<MorphTargetBinding> bindings = ResolveBindings(stage, time);
  if (bindings.empty()) {
    return false;
  }
  std::vector<BlendShape> shapes;
  std::vector<float> weights;
  shapes.reserve(bindings.size());
  weights.reserve(bindings.size());
  for (const MorphTargetBinding& binding : bindings) {
    BlendShape shape = BlendShape::ReadFromPrim(stage, binding.target_path);
    if (!shape) {
      continue;
    }
    shapes.push_back(std::move(shape));
    weights.push_back(binding.weight);
  }
  return ApplyMorphTargetsToPoints(base_points, shapes, weights, out, time);
}

bool ApplyMorphTargetsToPoints(const std::vector<freeusd::gf::Vec3f>& base_points,
                               const std::vector<BlendShape>& blend_shapes, const std::vector<float>& weights,
                               std::vector<freeusd::gf::Vec3f>* out, double time) {
  if (!out || base_points.empty() || blend_shapes.empty()) {
    return false;
  }
  *out = base_points;
  const std::size_t target_count = std::min(blend_shapes.size(), weights.size());
  for (std::size_t i = 0; i < target_count; ++i) {
    std::vector<freeusd::gf::Vec3f> offsets;
    std::vector<int> indices;
    if (!blend_shapes[i].GetOffsets(&offsets, time)) {
      continue;
    }
    (void)blend_shapes[i].GetPointIndices(&indices, time);
    accumulate_blend_shape(offsets, indices, weights[i], out);
  }
  return true;
}

}  // namespace freeusd::usdSkel
