#include "freeusd/usdUtils/engineScene.hpp"

#include <algorithm>
#include <unordered_set>

#include "freeusd/tf/token.hpp"
#include "freeusd/usdGeom/boundable.hpp"
#include "freeusd/usdGeom/imageable.hpp"
#include "freeusd/usdGeom/xformable.hpp"

namespace freeusd::usdUtils {
namespace {

void append_warning(std::vector<std::string>* warnings, std::unordered_set<std::string>* seen, std::string warning) {
  if (!warnings || !seen) {
    return;
  }
  if (seen->insert(warning).second) {
    warnings->push_back(std::move(warning));
  }
}

EngineSceneNode build_scene_node(const freeusd::usd::Prim& prim, double time) {
  EngineSceneNode node;
  if (!prim.IsValid()) {
    return node;
  }
  node.path = prim.GetPath();
  node.name = prim.GetName();
  node.active = prim.IsActive();
  node.visible = freeusd::usdGeom::Imageable(prim).ComputeVisibility(time);
  node.purpose = freeusd::usdGeom::Imageable(prim).ComputePurpose(time);
  node.local_transform = freeusd::usdGeom::Xformable(prim).ComputeLocalTransform(time);
  node.local_to_world_transform = freeusd::usdGeom::Xformable(prim).ComputeLocalToWorldTransform(time);
  node.local_bound = freeusd::usdGeom::Boundable(prim).ComputeLocalBound(time);
  node.world_bound = freeusd::usdGeom::Boundable(prim).ComputeWorldBound(time);
  if (prim.HasPrimKind()) {
    node.prim_kind = prim.GetPrimKind();
  }
  node.has_references = prim.HasReferences();
  node.has_payloads = prim.HasPayloads();
  node.has_inherits = prim.HasInherits();
  node.has_specializes = prim.HasSpecializes();
  node.variant_selection_sets = prim.ListVariantSelectionSets();
  node.has_variant_selections = !node.variant_selection_sets.empty();
  node.variant_set_names = prim.ListVariantSetNames();
  node.has_variant_sets = !node.variant_set_names.empty();
  node.composed_field_names = prim.ListAttributeNames();
  node.composed_relationship_names = prim.ListRelationshipNames();
  node.custom_data_keys = prim.ListCustomDataKeys();
  for (const std::string& field_name : node.composed_field_names) {
    if (!prim.ListAttributeSampleTimes(freeusd::tf::Token(field_name)).empty()) {
      node.has_time_samples = true;
      break;
    }
  }
  for (const freeusd::usd::Prim& child : prim.GetChildren()) {
    if (child.IsValid()) {
      node.child_paths.push_back(child.GetPath());
    }
  }
  return node;
}

}  // namespace

std::string_view EngineRuntimeModeName(EngineRuntimeMode mode) noexcept {
  switch (mode) {
    case EngineRuntimeMode::PreBakedAssetsOnly:
      return "pre_baked_assets_only";
    case EngineRuntimeMode::HybridMetadata:
      return "hybrid_metadata";
    case EngineRuntimeMode::ExperimentalLiveStage:
      return "experimental_live_stage";
  }
  return "pre_baked_assets_only";
}

EngineSceneSnapshot BuildEngineSceneSnapshot(const freeusd::usd::Stage& stage, double time) {
  EngineSceneSnapshot snapshot;
  snapshot.pseudo_root_path = stage.GetPseudoRootPath();
  snapshot.root_identifier = stage.GetRootLayer().GetIdentifier();
  if (stage.HasDefaultPrim()) {
    snapshot.default_prim_name = stage.GetDefaultPrimName();
  }
  snapshot.start_time_code = stage.GetStartTimeCode();
  snapshot.end_time_code = stage.GetEndTimeCode();
  snapshot.time_codes_per_second = stage.GetTimeCodesPerSecond();
  snapshot.frames_per_second = stage.GetFramesPerSecond();
  snapshot.frame_precision = stage.GetFramePrecision();
  snapshot.meters_per_unit = stage.GetMetersPerUnit();
  snapshot.up_axis = stage.GetUpAxis();
  snapshot.prim_order = stage.GetPrimOrder();
  stage.TraversePreorder([&snapshot, time](const freeusd::usd::Prim& prim) {
    snapshot.nodes.push_back(build_scene_node(prim, time));
    return true;
  });
  return snapshot;
}

EnginePrimEditorView BuildEnginePrimEditorView(const freeusd::usd::Prim& prim, double time) {
  EnginePrimEditorView view;
  if (!prim.IsValid()) {
    return view;
  }
  view.path = prim.GetPath();
  view.composed_field_names = prim.ListAttributeNames();
  for (const std::string& field_name : view.composed_field_names) {
    const std::vector<double> sample_times = prim.ListAttributeSampleTimes(freeusd::tf::Token(field_name));
    if (!sample_times.empty()) {
      view.attribute_sample_times.emplace_back(field_name, sample_times);
    }
  }
  view.composed_relationship_names = prim.ListRelationshipNames();
  for (const std::string& rel_name : view.composed_relationship_names) {
    view.relationship_targets.emplace_back(rel_name, prim.GetRelationshipTargets(freeusd::tf::Token(rel_name)));
  }
  view.custom_data_keys = prim.ListCustomDataKeys();
  view.variant_selection_sets = prim.ListVariantSelectionSets();
  view.variant_set_names = prim.ListVariantSetNames();
  (void)time;
  return view;
}

EngineRuntimeSupportReport AssessEngineRuntimeSupport(const freeusd::usd::Stage& stage) {
  EngineRuntimeSupportReport report;
  report.recommended_mode = EngineRuntimeMode::ExperimentalLiveStage;
  report.uses_composed_layer_stack = stage.GetComposeLayers().size() > 1u;
  report.uses_relocates = !stage.ListComposedRelocates().empty();
  report.uses_prefix_substitutions = !stage.ListComposedPrefixSubstitutions().empty();

  std::unordered_set<std::string> seen_warnings;
  if (report.uses_composed_layer_stack) {
    append_warning(&report.warnings, &seen_warnings,
                   "Scene uses multiple composed layers; prefer pre-baked assets for shipped runtime.");
  }
  if (report.uses_relocates) {
    append_warning(&report.warnings, &seen_warnings,
                   "Scene uses relocates; resolve namespace changes during offline import.");
  }
  if (report.uses_prefix_substitutions) {
    append_warning(&report.warnings, &seen_warnings,
                   "Scene uses prefix substitutions; bake resolved asset paths into runtime assets.");
  }

  stage.TraversePreorder([&](const freeusd::usd::Prim& prim) {
    if (!prim.IsValid()) {
      return true;
    }
    report.uses_references = report.uses_references || prim.HasReferences();
    report.uses_payloads = report.uses_payloads || prim.HasPayloads();
    report.uses_inherits = report.uses_inherits || prim.HasInherits();
    report.uses_specializes = report.uses_specializes || prim.HasSpecializes();
    report.uses_variant_selection = report.uses_variant_selection || !prim.ListVariantSelectionSets().empty();
    report.uses_variant_sets = report.uses_variant_sets || !prim.ListVariantSetNames().empty();
    report.uses_relationships = report.uses_relationships || !prim.ListRelationshipNames().empty();
    report.uses_custom_data = report.uses_custom_data || !prim.ListCustomDataKeys().empty();
    for (const std::string& field_name : prim.ListAttributeNames()) {
      const freeusd::tf::Token token(field_name);
      report.uses_attribute_connections =
          report.uses_attribute_connections || prim.HasAttributeConnection(token);
      report.uses_time_samples = report.uses_time_samples || !prim.ListAttributeSampleTimes(token).empty();
    }
    return true;
  });

  if (report.uses_references) {
    append_warning(&report.warnings, &seen_warnings,
                   "Scene uses references; resolve composed assets during offline import.");
  }
  if (report.uses_payloads) {
    append_warning(&report.warnings, &seen_warnings,
                   "Scene uses payloads; prefer pre-baked runtime assets instead of live stage evaluation.");
  }
  if (report.uses_inherits || report.uses_specializes) {
    append_warning(&report.warnings, &seen_warnings,
                   "Scene uses inherits or specializes; keep runtime on imported engine assets.");
  }
  if (report.uses_variant_selection || report.uses_variant_sets) {
    append_warning(&report.warnings, &seen_warnings,
                   "Scene uses variants; select and bake runtime variants during import.");
  }
  if (report.uses_time_samples) {
    append_warning(&report.warnings, &seen_warnings,
                   "Scene uses time samples; pre-bake sampled values or stream them through a controlled runtime path.");
  }

  const bool requires_prebake = report.uses_composed_layer_stack || report.uses_references || report.uses_payloads ||
                                report.uses_inherits || report.uses_specializes || report.uses_variant_selection ||
                                report.uses_variant_sets || report.uses_relocates || report.uses_prefix_substitutions ||
                                report.uses_time_samples;
  if (requires_prebake) {
    report.recommended_mode = EngineRuntimeMode::PreBakedAssetsOnly;
  } else if (report.uses_relationships || report.uses_custom_data || report.uses_attribute_connections) {
    append_warning(&report.warnings, &seen_warnings,
                   "Scene uses composed metadata beyond static transforms; limit runtime usage to hybrid metadata reads.");
    report.recommended_mode = EngineRuntimeMode::HybridMetadata;
  }
  return report;
}

}  // namespace freeusd::usdUtils
