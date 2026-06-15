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
#include "freeusd/usdLux/tokens.hpp"
#include "freeusd/usdPhysics/collisionAPI.hpp"
#include "freeusd/usdPhysics/fixedJoint.hpp"
#include "freeusd/usdPhysics/massAPI.hpp"
#include "freeusd/usdPhysics/rigidBodyAPI.hpp"
#include "freeusd/usdPhysics/tokens.hpp"
#include "freeusd/usdSemantics/labelsAPI.hpp"
#include "freeusd/usdVol/openVdbAsset.hpp"
#include "freeusd/usdVol/volume.hpp"
#include "freeusd/usdVol/tokens.hpp"
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

bool is_supported_lux_light_kind(const freeusd::tf::Token& kind) {
  using namespace freeusd::usdLux::tokens;
  return kind == DistantLight() || kind == SphereLight() || kind == RectLight() || kind == DiskLight() ||
         kind == CylinderLight() || kind == DomeLight();
}

bool preview_surface_has_textured_inputs(const freeusd::usdShade::PreviewSurface& preview, double time) {
  if (!preview) {
    return false;
  }
  std::string path;
  return (preview.GetDiffuseTextureAssetPath(&path, time) && !path.empty()) ||
         (preview.GetNormalTextureAssetPath(&path, time) && !path.empty()) ||
         (preview.GetOcclusionTextureAssetPath(&path, time) && !path.empty()) ||
         (preview.GetMetallicTextureAssetPath(&path, time) && !path.empty()) ||
         (preview.GetRoughnessTextureAssetPath(&path, time) && !path.empty());
}

EngineSceneNode build_scene_node(const std::shared_ptr<const freeusd::usd::Stage>& stage_ptr,
                                 const freeusd::usd::Prim& prim, double time) {
  EngineSceneNode node;
  if (!prim.IsValid()) {
    return node;
  }
  node.path = prim.GetPath();
  node.name = prim.GetName();
  node.has_prim_kind = prim.HasPrimKind();
  if (node.has_prim_kind) {
    node.prim_kind = prim.GetPrimKind();
  }
  node.has_prim_active_opinion = prim.HasPrimActiveOpinion();
  node.active = prim.IsActive();
  node.visible = freeusd::usdGeom::Imageable(prim).ComputeVisibility(time);
  node.purpose = freeusd::usdGeom::Imageable(prim).ComputePurpose(time);
  node.local_transform = freeusd::usdGeom::Xformable(prim).ComputeLocalTransform(time);
  node.local_to_world_transform = freeusd::usdGeom::Xformable(prim).ComputeLocalToWorldTransform(time);
  node.local_bound = freeusd::usdGeom::Boundable(prim).ComputeLocalBound(time);
  node.world_bound = freeusd::usdGeom::Boundable(prim).ComputeWorldBound(time);
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
    if (stage_ptr) {
      const freeusd::usdShade::Material material =
          freeusd::usdShade::Material::ReadFromPrim(stage_ptr, node.material_path);
      const freeusd::sdf::Path shader_path = material.GetSurfaceShaderPath();
      if (!shader_path.IsEmpty()) {
        const freeusd::usdShade::PreviewSurface preview =
            freeusd::usdShade::PreviewSurface::ReadFromPrim(stage_ptr, shader_path);
        node.has_preview_surface_textures = preview_surface_has_textured_inputs(preview, time);
      }
    }
  }
  if (prim.HasPrimKind() && is_supported_lux_light_kind(prim.GetPrimKind())) {
    node.has_lux_light = true;
    node.lux_light_type = prim.GetPrimKind().GetText();
  }
  if (prim.HasPrimKind() && prim.GetPrimKind() == freeusd::usdPhysics::tokens::PhysicsScene()) {
    node.has_physics_scene = true;
  }
  if (stage_ptr) {
    const freeusd::usdVol::OpenVDBAsset open_vdb =
        freeusd::usdVol::OpenVDBAsset::ReadFromPrim(stage_ptr, prim.GetPath());
    if (open_vdb.IsOpenVDBAsset()) {
      node.has_open_vdb_asset = true;
      (void)open_vdb.GetFilePath(&node.open_vdb_file_path, time);
      (void)open_vdb.GetFieldName(&node.open_vdb_field_name, time);
    }
    const freeusd::usdVol::Volume volume = freeusd::usdVol::Volume::ReadFromPrim(stage_ptr, prim.GetPath());
    if (volume.IsVolume()) {
      node.has_volume = true;
      node.volume_field_asset_paths = volume.GetFieldRelationshipTargets();
    }
    const freeusd::usdPhysics::RigidBodyAPI rigid_body =
        freeusd::usdPhysics::RigidBodyAPI::ReadFromPrim(stage_ptr, prim.GetPath());
    if (rigid_body.IsRigidBodyAPI() && prim.IsInstancePrim()) {
      node.has_rigid_body_api = true;
    }
    const freeusd::usdPhysics::CollisionAPI collision =
        freeusd::usdPhysics::CollisionAPI::ReadFromPrim(stage_ptr, prim.GetPath());
    if (collision.IsCollisionAPI() && prim.IsInstancePrim()) {
      node.has_collision_api = true;
    }
    const freeusd::usdPhysics::FixedJoint joint =
        freeusd::usdPhysics::FixedJoint::ReadFromPrim(stage_ptr, prim.GetPath());
    if (joint.IsFixedJoint() && prim.IsInstancePrim()) {
      node.has_physics_fixed_joint = true;
      (void)joint.GetBody0(&node.physics_fixed_joint_body0);
      (void)joint.GetBody1(&node.physics_fixed_joint_body1);
    }
    const freeusd::usdSemantics::SemanticLabelsAPI semantic_labels =
        freeusd::usdSemantics::SemanticLabelsAPI::ReadFromPrim(stage_ptr, prim.GetPath());
    node.semantic_label_set_names = semantic_labels.ListLabelSetNames();
    node.has_semantic_labels = !node.semantic_label_set_names.empty();
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
    EngineSceneNode node = build_scene_node(stage_ptr, prim, time);
    if (node.has_skel_binding) {
      snapshot.skel_bound_geom_paths.push_back(node.path);
    }
    if (node.has_blend_shapes) {
      snapshot.blend_shape_geom_paths.push_back(node.path);
    }
    if (node.has_material_binding) {
      snapshot.material_bound_geom_paths.push_back(node.path);
    }
    if (node.has_lux_light) {
      append_unique_path(&snapshot.lux_light_paths, node.path);
    }
    if (node.has_physics_scene) {
      append_unique_path(&snapshot.physics_scene_paths, node.path);
    }
    if (node.has_rigid_body_api) {
      append_unique_path(&snapshot.rigid_body_api_paths, node.path);
    }
    if (node.has_collision_api) {
      append_unique_path(&snapshot.collision_api_paths, node.path);
    }
    if (node.has_physics_fixed_joint) {
      append_unique_path(&snapshot.physics_fixed_joint_paths, node.path);
    }
    if (node.has_open_vdb_asset) {
      append_unique_path(&snapshot.open_vdb_asset_paths, node.path);
    }
    if (node.has_volume) {
      append_unique_path(&snapshot.volume_paths, node.path);
    }
    if (node.has_semantic_labels) {
      append_unique_path(&snapshot.semantic_label_prim_paths, node.path);
    }
    if (node.has_prim_kind) {
      append_unique_path(&snapshot.composed_kind_prim_paths, node.path);
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
          if (preview_surface_has_textured_inputs(preview, time)) {
            append_unique_path(&snapshot.preview_surface_textured_shader_paths, shader_path);
          }
        }
      }
    }
    if (prim.HasPrimKind() && prim.GetPrimKind() == freeusd::usdShade::tokens::Shader()) {
      const freeusd::usdShade::PreviewSurface preview_shader =
          freeusd::usdShade::PreviewSurface::ReadFromPrim(stage_ptr, prim.GetPath());
      if (preview_shader) {
        append_unique_path(&snapshot.preview_surface_shader_paths, prim.GetPath());
        if (preview_surface_has_textured_inputs(preview_shader, time)) {
          append_unique_path(&snapshot.preview_surface_textured_shader_paths, prim.GetPath());
        }
      }
    }
    snapshot.nodes.push_back(std::move(node));
    return true;
  });
  return snapshot;
}

std::vector<EngineSpatialGroundingRecord> BuildEngineSpatialGroundingContext(const freeusd::usd::Stage& stage,
                                                                             double time) {
  std::vector<EngineSpatialGroundingRecord> records;
  const std::shared_ptr<const freeusd::usd::Stage> stage_ptr = stage.shared_from_this();
  stage.TraversePreorder([&records, &stage, &stage_ptr, time](const freeusd::usd::Prim& prim) {
    if (!prim.IsValid()) {
      return true;
    }
    EngineSpatialGroundingRecord record;
    record.path = prim.GetPath();
    record.name = prim.GetName();
    record.parent_path = record.path.GetParentPath();
    const freeusd::usd::Prim parent = stage.GetPrimAtPath(record.parent_path);
    if (parent.IsValid()) {
      for (const freeusd::usd::Prim& sibling : parent.GetChildren()) {
        if (sibling.IsValid() && sibling.GetPath() != record.path) {
          record.sibling_names.push_back(sibling.GetName());
        }
      }
    }

    const freeusd::gf::Matrix4d local_to_world =
        freeusd::usdGeom::Xformable(prim).ComputeLocalToWorldTransform(time);
    record.world_position.set(local_to_world.m[12], local_to_world.m[13], local_to_world.m[14]);

    const freeusd::gf::BBox3d world_bound = freeusd::usdGeom::Boundable(prim).ComputeWorldBound(time);
    if (!world_bound.IsEmpty()) {
      record.has_world_bound = true;
      record.world_bound_dimensions.set(world_bound.max.x() - world_bound.min.x(),
                                         world_bound.max.y() - world_bound.min.y(),
                                         world_bound.max.z() - world_bound.min.z());
    }

    float mass = 0.0F;
    const freeusd::usdPhysics::RigidBodyAPI rigid_body =
        freeusd::usdPhysics::RigidBodyAPI::ReadFromPrim(stage_ptr, record.path);
    if (rigid_body.GetMass(&mass, time)) {
      record.mass_kg = static_cast<double>(mass);
    } else {
      const freeusd::usdPhysics::MassAPI mass_api = freeusd::usdPhysics::MassAPI::ReadFromPrim(stage_ptr, record.path);
      if (mass_api.IsMassAPI() && prim.GetAttribute(freeusd::usdPhysics::tokens::physics_mass(), time).GetFloat(&mass)) {
        record.mass_kg = static_cast<double>(mass);
      }
    }

    records.push_back(std::move(record));
    return true;
  });
  return records;
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
    if (prim.HasPrimKind() && is_supported_lux_light_kind(prim.GetPrimKind())) {
      report.uses_lux_lights = true;
    }
    if (prim.HasPrimKind() && prim.GetPrimKind() == freeusd::usdPhysics::tokens::PhysicsScene()) {
      report.uses_physics_scenes = true;
    }
    if (prim.IsInstancePrim() && freeusd::usdPhysics::RigidBodyAPI(prim).IsRigidBodyAPI()) {
      report.uses_rigid_body_api = true;
    }
    if (prim.IsInstancePrim() && freeusd::usdPhysics::CollisionAPI(prim).IsCollisionAPI()) {
      report.uses_collision_api = true;
    }
    if (prim.IsInstancePrim() && freeusd::usdPhysics::FixedJoint(prim).IsFixedJoint()) {
      report.uses_physics_fixed_joints = true;
    }
    if (freeusd::usdVol::OpenVDBAsset(prim).IsOpenVDBAsset()) {
      report.uses_open_vdb_assets = true;
    }
    if (freeusd::usdVol::Volume(prim).IsVolume()) {
      report.uses_volumes = true;
    }
    const freeusd::usdSemantics::SemanticLabelsAPI semantic_labels =
        freeusd::usdSemantics::SemanticLabelsAPI::ReadFromPrim(stage.shared_from_this(), prim.GetPath());
    report.uses_semantic_labels = report.uses_semantic_labels || !semantic_labels.ListLabelSetNames().empty();
    report.uses_composed_prim_kind = report.uses_composed_prim_kind || prim.HasPrimKind();
    report.uses_prim_active_opinions = report.uses_prim_active_opinions || prim.HasPrimActiveOpinion();
    const bool metadata_arc_prim =
        prim.HasReferences() || prim.HasPayloads() || prim.HasInherits() || prim.HasSpecializes();
    if (metadata_arc_prim && (prim.HasPrimKind() || prim.HasPrimActiveOpinion())) {
      report.uses_kind_active_through_arcs = true;
    }
    if (metadata_arc_prim && !prim.ListCustomDataKeys().empty()) {
      report.uses_custom_data_through_arcs = true;
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
        const freeusd::usdShade::PreviewSurface preview =
            freeusd::usdShade::PreviewSurface::ReadFromPrim(stage.shared_from_this(), shader_path);
        report.uses_preview_surface = report.uses_preview_surface || static_cast<bool>(preview);
        report.uses_preview_surface_textures =
            report.uses_preview_surface_textures || preview_surface_has_textured_inputs(preview, 1.0);
      }
    }
    if (prim.HasPrimKind() && prim.GetPrimKind() == freeusd::usdShade::tokens::Shader()) {
      const freeusd::usdShade::PreviewSurface preview_shader =
          freeusd::usdShade::PreviewSurface::ReadFromPrim(stage.shared_from_this(), prim.GetPath());
      report.uses_preview_surface = report.uses_preview_surface || static_cast<bool>(preview_shader);
      report.uses_preview_surface_textures =
          report.uses_preview_surface_textures || preview_surface_has_textured_inputs(preview_shader, 1.0);
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
  if (report.uses_lux_lights) {
    append_warning(&report.warnings, &seen_warnings,
                   "Scene uses usdLux lights; bake light parameters into engine-native light assets.");
  }
  if (report.uses_preview_surface_textures) {
    append_warning(&report.warnings, &seen_warnings,
                   "Scene uses preview-surface texture assets; resolve and bake textures during offline import.");
  }
  if (report.uses_kind_active_through_arcs) {
    append_warning(&report.warnings, &seen_warnings,
                   "Scene resolves prim kind or active through composition arcs (references, payloads, inherits, or "
                   "specializes); bake hierarchy metadata during import.");
  }
  if (report.uses_custom_data_through_arcs) {
    append_warning(&report.warnings, &seen_warnings,
                   "Scene resolves prim customData through composition arcs; bake editor metadata during import.");
  }
  if (report.uses_physics_scenes) {
    append_warning(&report.warnings, &seen_warnings,
                   "Scene uses PhysicsScene prims; bake gravity and simulation settings during offline import.");
  }
  if (report.uses_rigid_body_api) {
    append_warning(&report.warnings, &seen_warnings,
                   "Scene uses PhysicsRigidBodyAPI prims; bake rigid-body mass during offline import.");
  }
  if (report.uses_collision_api) {
    append_warning(&report.warnings, &seen_warnings,
                   "Scene uses PhysicsCollisionAPI prims; bake collision enabled flags during offline import.");
  }
  if (report.uses_physics_fixed_joints) {
    append_warning(&report.warnings, &seen_warnings,
                   "Scene uses PhysicsFixedJoint prims; bake joint body attachments during offline import.");
  }
  if (report.uses_open_vdb_assets) {
    append_warning(&report.warnings, &seen_warnings,
                   "Scene uses OpenVDBAsset prims; resolve VDB files and field names during offline import.");
  }
  if (report.uses_volumes) {
    append_warning(&report.warnings, &seen_warnings,
                   "Scene uses Volume prims; resolve field assets and VDB paths during offline import.");
  }
  if (report.uses_semantic_labels) {
    append_warning(&report.warnings, &seen_warnings,
                   "Scene uses semantic labels; bake or validate label mappings during import.");
  }

  const bool requires_prebake = report.uses_composed_layer_stack || report.uses_references || report.uses_payloads ||
                                report.uses_inherits || report.uses_specializes || report.uses_variant_selection ||
                                report.uses_variant_sets || report.uses_relocates || report.uses_prefix_substitutions ||
                                report.uses_time_samples;
  if (requires_prebake) {
    report.recommended_mode = EngineRuntimeMode::PreBakedAssetsOnly;
  } else if (report.uses_relationships || report.uses_custom_data || report.uses_attribute_connections ||
             report.uses_semantic_labels) {
    append_warning(&report.warnings, &seen_warnings,
                   "Scene uses composed metadata beyond static transforms; limit runtime usage to hybrid metadata reads.");
    report.recommended_mode = EngineRuntimeMode::HybridMetadata;
  }
  return report;
}

}  // namespace freeusd::usdUtils
