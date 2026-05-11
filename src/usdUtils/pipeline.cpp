#include "freeusd/usdUtils/pipeline.hpp"

#include <utility>

#include "freeusd/sdf/layer.hpp"
#include "freeusd/tf/token.hpp"
#include "freeusd/usd/prim.hpp"
#include "freeusd/usd/stage.hpp"

namespace freeusd::usdUtils {
namespace {

void copy_stage_metadata(const freeusd::usd::Stage& stage, freeusd::sdf::Layer& out, const FlattenOptions& options) {
  if (!options.merge_authored_layer_metadata) {
    return;
  }
  if (stage.HasDefaultPrim()) {
    out.SetDefaultPrim(stage.GetDefaultPrimName());
  }
  if (const auto v = stage.GetStartTimeCode(); v.has_value()) {
    out.SetStartTimeCode(*v);
  }
  if (const auto v = stage.GetEndTimeCode(); v.has_value()) {
    out.SetEndTimeCode(*v);
  }
  if (const auto v = stage.GetTimeCodesPerSecond(); v.has_value()) {
    out.SetTimeCodesPerSecond(*v);
  }
  if (const auto v = stage.GetFramesPerSecond(); v.has_value()) {
    out.SetFramesPerSecond(*v);
  }
  if (const auto v = stage.GetFramePrecision(); v.has_value()) {
    out.SetFramePrecision(*v);
  }
  if (const auto v = stage.GetMetersPerUnit(); v.has_value()) {
    out.SetMetersPerUnit(*v);
  }
  if (const auto v = stage.GetUpAxis(); v.has_value()) {
    out.SetUpAxis(*v);
  }
  out.SetPrimOrder(stage.GetPrimOrder());
  for (const auto& pr : stage.ListComposedRelocates()) {
    out.SetRelocate(pr.first, pr.second);
  }
  for (const auto& pr : stage.ListComposedPrefixSubstitutions()) {
    out.SetPrefixSubstitution(pr.first, pr.second);
  }
  for (const std::string& key : stage.ListComposedCustomLayerDataKeys()) {
    freeusd::vt::Value v;
    if (stage.GetComposedCustomLayerData(key, &v) && !v.IsEmpty()) {
      out.SetCustomLayerDataEntry(key, v);
    }
  }
}

void copy_prim_at_time(const freeusd::usd::Prim& prim, double time, freeusd::sdf::Layer& out) {
  if (!prim.IsValid()) {
    return;
  }
  const freeusd::sdf::Path path = prim.GetPath();
  if (prim.HasPrimKind()) {
    out.SetPrimKind(path, prim.GetPrimKind());
  }
  if (prim.HasPrimActiveOpinion()) {
    out.SetPrimActive(path, prim.IsActive());
  }
  const auto spec = prim.GetSpecifierKind();
  if (spec != freeusd::sdf::Layer::PrimSpecifierKind::Default &&
      spec != freeusd::sdf::Layer::PrimSpecifierKind::Def) {
    out.SetPrimSpecifier(path, spec);
  }
  for (const std::string& key : prim.ListCustomDataKeys()) {
    const freeusd::vt::Value v = prim.GetCustomData(key);
    if (!v.IsEmpty()) {
      out.SetPrimCustomDataEntry(path, key, v);
    }
  }
  for (const std::string& variant_set : prim.ListVariantSelectionSets()) {
    const std::string selected = prim.GetVariantSelection(variant_set);
    if (!selected.empty()) {
      out.SetPrimVariantSelectionEntry(path, variant_set, selected);
    }
  }
  for (const std::string& variant_set : prim.ListVariantSetNames()) {
    const std::vector<std::string> names = prim.ListVariantNames(variant_set);
    if (!names.empty()) {
      out.SetPrimVariantSetVariants(path, variant_set, names);
    }
  }
  const std::vector<freeusd::sdf::PrimReference> refs = prim.GetReferences();
  if (!refs.empty()) {
    out.SetPrimReferences(path, refs);
  }
  const std::vector<freeusd::sdf::Path> inherits = prim.GetInherits();
  if (!inherits.empty()) {
    out.SetPrimInherits(path, inherits);
  }
  const std::vector<freeusd::sdf::Path> specializes = prim.GetSpecializes();
  if (!specializes.empty()) {
    out.SetPrimSpecializes(path, specializes);
  }
  const std::vector<freeusd::sdf::PrimReference> payloads = prim.GetPayloads();
  if (!payloads.empty()) {
    out.SetPrimPayloads(path, payloads);
  }
  for (const std::string& rel_name : prim.ListRelationshipNames()) {
    const std::vector<freeusd::sdf::Path> targets = prim.GetRelationshipTargets(freeusd::tf::Token(rel_name));
    if (!targets.empty()) {
      out.SetRelationshipTargets(path, freeusd::tf::Token(rel_name), targets);
    }
  }
  for (const std::string& attr_name : prim.ListAttributeNames()) {
    const freeusd::tf::Token token(attr_name);
    freeusd::sdf::Path conn_target;
    if (prim.GetAttributeConnectionTarget(token, &conn_target)) {
      out.SetAttributeConnection(path, token, conn_target);
    }
    const freeusd::vt::Value v = prim.GetAttribute(token, time);
    if (!v.IsEmpty()) {
      out.SetField(path, token, v);
    }
  }
}

}  // namespace

std::shared_ptr<freeusd::sdf::Layer> FlattenStageAtTime(const freeusd::usd::Stage& stage, double time,
                                                        const FlattenOptions& options) {
  auto out = freeusd::sdf::Layer::NewAnonymous("flattened");
  copy_stage_metadata(stage, *out, options);
  stage.TraversePreorder([&](const freeusd::usd::Prim& prim) {
    copy_prim_at_time(prim, time, *out);
    return true;
  });
  return out;
}

}  // namespace freeusd::usdUtils
