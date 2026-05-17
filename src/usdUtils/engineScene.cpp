#include "freeusd/usdUtils/engineScene.hpp"

#include <algorithm>
#include <unordered_set>

#include "freeusd/tf/token.hpp"
#include "freeusd/usdGeom/boundable.hpp"
#include "freeusd/usdGeom/imageable.hpp"
#include "freeusd/usdGeom/xformable.hpp"
#include "freeusd/usdShade/material.hpp"
#include "freeusd/usdShade/previewSurface.hpp"
#include "freeusd/usdShade/tokens.hpp"
#include "freeusd/usdSkel/skelBinding.hpp"
#include "freeusd/usdSkel/skelBlendShapes.hpp"
#include "freeusd/usdSkel/tokens.hpp"

namespace freeusd::usdUtils {
namespace {

const freeusd::tf::Token& material_binding_token() {
  static const freeusd::tf::Token kMaterialBinding("material:binding");
  return kMaterialBinding;
}

void append_unique_path(std::vector<freeusd::sdf::Path>* paths, const freeusd::sdf::Path& path) {
  if (!paths || path.IsEmpty()) {
    return;
  }
  if (std::find(paths->begin(), paths->end(), path) == paths->end()) {
    paths->push_back(path);
  }
}

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
  if (const std::optional<freeusd::sdf::Path> skeleton_path = freeusd::usdSkel::SkelBinding::ResolveSkeletonPath(prim)) {
    node.has_skel_binding = true;
    node.skel_skeleton_path = *skeleton_path;
  }
  const freeusd::usdSkel::SkelBlendShapes blend_shapes = freeusd::usdSkel::SkelBlendShapes::ReadFromGeomPrim(prim);
  if (blend_shapes) {
    node.has_blend_shapes = true;
    node.blend_shape_tokens = blend_shapes.blend_shape_tokens;
  }
  const std::vector<freeusd::sdf::Path> material_bindings = prim.GetRelationshipTargets(material_binding_token());
  if (!material_bindings.empty()) {
    node.has_material_binding = true;
    node.material_path = material_bindings.front();
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
  const std::shared_ptr<const freeusd::usd::Stage> stage_ptr = stage.shared_from_this();
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
  stage.TraversePreorder([stage_ptr, &snapshot, time](const freeusd::usd::Prim& prim) {
    EngineSceneNode node = build_scene_node(prim, time);
    if (node.has_skel_binding) {
      snapshot.skel_bound_geom_paths.push_back(node.path);
    }
    if (node.has_blend_shapes) {
      snapshot.blend_shape_geom_paths.push_back(node.path);
    }
    if (node.has_material_binding) {
      snapshot.material_bound_geom_paths.push_back(node.path);
    }
    if (prim.HasPrimKind() && prim.GetPrimKind() == freeusd::usdSkel::tokens::SkelRoot()) {
      snapshot.skel_root_paths.push_back(node.path);
    }
    if (prim.HasPrimKind() && prim.GetPrimKind() == freeusd::usdSkel::tokens::SkelAnimation()) {
      snapshot.skel_animation_paths.push_back(node.path);
    }
    if (prim.HasPrimKind() && prim.GetPrimKind() == freeusd::usdShade::tokens::Material()) {
      const freeusd::usdShade::Material material =
          freeusd::usdShade::Material::ReadFromPrim(stage_ptr, prim.GetPath());
      const freeusd::sdf::Path shader_path = material.GetSurfaceShaderPath();
      if (!shader_path.IsEmpty()) {
        const freeusd::usdShade::PreviewSurface preview =
            freeusd::usdShade::PreviewSurface::ReadFromPrim(stage_ptr, shader_path);
        if (preview) {
          append_unique_path(&snapshot.material_paths, prim.GetPath());
          append_unique_path(&snapshot.preview_surface_shader_paths, shader_path);
        }
      }
    }
    if (prim.HasPrimKind() && prim.GetPrimKind() == freeusd::usdShade::tokens::Shader()) {
      const freeusd::usdShade::PreviewSurface preview_shader =
          freeusd::usdShade::PreviewSurface::ReadFromPrim(stage_ptr, prim.GetPath());
      if (preview_shader) {
        append_unique_path(&snapshot.preview_surface_shader_paths, prim.GetPath());
      }
    }
    snapshot.nodes.push_back(std::move(node));
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

  stage.TraversePreorder([&report, &stage](const freeusd::usd::Prim& prim) {
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
    report.uses_skel_bound_meshes =
        report.uses_skel_bound_meshes || freeusd::usdSkel::SkelBinding::ResolveSkeletonPath(prim).has_value();
    const freeusd::usdSkel::SkelBlendShapes blend_shapes = freeusd::usdSkel::SkelBlendShapes::ReadFromGeomPrim(prim);
    report.uses_blend_shapes = report.uses_blend_shapes || static_cast<bool>(blend_shapes);
    if (prim.HasPrimKind() && prim.GetPrimKind() == freeusd::usdSkel::tokens::SkelAnimation()) {
      report.uses_skel_animation = true;
    }
    if (prim.HasPrimKind() && prim.GetPrimKind() == freeusd::usdSkel::tokens::SkelRoot()) {
      const std::vector<freeusd::sdf::Path> anim_targets =
          prim.GetRelationshipTargets(freeusd::usdSkel::tokens::skel_animationSource());
      report.uses_skel_animation = report.uses_skel_animation || !anim_targets.empty();
    }
    const std::vector<freeusd::sdf::Path> material_bindings = prim.GetRelationshipTargets(material_binding_token());
    if (!material_bindings.empty()) {
      report.uses_material_bindings = true;
    }
    if (prim.HasPrimKind() && prim.GetPrimKind() == freeusd::usdShade::tokens::Material()) {
      const freeusd::sdf::Path shader_path = freeusd::usdShade::Material(prim).GetSurfaceShaderPath();
      if (!shader_path.IsEmpty()) {
        const freeusd::usdShade::PreviewSurface preview(
            freeusd::usdShade::Shader(stage.GetPrimAtPath(shader_path)));
        report.uses_preview_surface = report.uses_preview_surface || static_cast<bool>(preview);
      }
    }
    if (prim.HasPrimKind() && prim.GetPrimKind() == freeusd::usdShade::tokens::Shader()) {
      const freeusd::usdShade::PreviewSurface preview_shader(freeusd::usdShade::Shader(prim));
      report.uses_preview_surface = report.uses_preview_surface || static_cast<bool>(preview_shader);
    }
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
  if (report.uses_skel_bound_meshes) {
    append_warning(&report.warnings, &seen_warnings,
                   "Scene uses skeletal skinning; bake deformations or evaluate through a controlled skinning path.");
  }
  if (report.uses_blend_shapes) {
    append_warning(&report.warnings, &seen_warnings,
                   "Scene uses blend shapes; bake morph targets or stream weights through a controlled runtime path.");
  }
  if (report.uses_skel_animation) {
    append_warning(&report.warnings, &seen_warnings,
                   "Scene uses skeletal animation clips; pre-bake joint samples for shipping runtime.");
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
