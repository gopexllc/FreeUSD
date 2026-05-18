#include <algorithm>
#include <cassert>
#include <string>

#include "freeusd/sdf/path.hpp"
#include "freeusd/usd/stage.hpp"
#include "freeusd/usdUtils/engineScene.hpp"

#ifndef FREEUSD_TEST_FIXTURES_DIR
#error "FREEUSD_TEST_FIXTURES_DIR must be set by CMake for engine_integration_test"
#endif

namespace {

std::string fixture(const char* name) {
  return std::string(FREEUSD_TEST_FIXTURES_DIR) + "/" + name;
}

const freeusd::usdUtils::EngineSceneNode* find_node(const freeusd::usdUtils::EngineSceneSnapshot& snapshot,
                                                    const freeusd::sdf::Path& path) {
  for (const auto& node : snapshot.nodes) {
    if (node.path == path) {
      return &node;
    }
  }
  return nullptr;
}

}  // namespace

int main() {
  using freeusd::sdf::Path;
  using freeusd::usd::RootLayerSublayersPolicy;
  using freeusd::usd::Stage;
  using freeusd::usdUtils::AssessEngineRuntimeSupport;
  using freeusd::usdUtils::BuildEnginePrimEditorView;
  using freeusd::usdUtils::BuildEngineSceneSnapshot;
  using freeusd::usdUtils::EngineRuntimeMode;
  using freeusd::usdUtils::EngineRuntimeModeName;

  {
    std::string err;
    auto stage = Stage::OpenFromRootFile(fixture("parity_imageable.usda"), RootLayerSublayersPolicy::DepthFirst, &err);
    assert(stage && err.empty());
    const auto snapshot = BuildEngineSceneSnapshot(*stage, 1.0);
    assert(snapshot.default_prim_name == "World");
    assert(snapshot.root_identifier.find("parity_imageable.usda") != std::string::npos);
    const auto* world = find_node(snapshot, Path::FromString("/World"));
    const auto* cube = find_node(snapshot, Path::FromString("/World/Cube"));
    assert(world != nullptr);
    assert(cube != nullptr);
    assert(world->child_paths.size() == 1u);
    assert(world->child_paths[0] == Path::FromString("/World/Cube"));
    assert(cube->purpose == "render");
    assert(!cube->visible);
    assert(!cube->world_bound.IsEmpty());
    assert(cube->world_bound.min.x() == 0.0 && cube->world_bound.min.y() == 1.0 && cube->world_bound.min.z() == 2.0);
    assert(cube->world_bound.max.x() == 2.0 && cube->world_bound.max.y() == 3.0 && cube->world_bound.max.z() == 4.0);

    const auto report = AssessEngineRuntimeSupport(*stage);
    assert(report.recommended_mode == EngineRuntimeMode::ExperimentalLiveStage);
    assert(std::string(EngineRuntimeModeName(report.recommended_mode)) == "experimental_live_stage");
  }

  {
    std::string err;
    auto stage = Stage::OpenFromRootFile(fixture("parity_stack_root.usda"), RootLayerSublayersPolicy::DepthFirst, &err);
    assert(stage && err.empty());
    const auto prim = stage->GetPrimAtPath(Path::FromString("/World/Model"));
    const auto editor = BuildEnginePrimEditorView(prim, 15.0);
    assert(editor.path == Path::FromString("/World/Model"));
    bool saw_stacked_only = false;
    for (const auto& name : editor.composed_field_names) {
      if (name == "stackedOnly") {
        saw_stacked_only = true;
        break;
      }
    }
    assert(saw_stacked_only);
    bool saw_stacked_only_samples = false;
    for (const auto& item : editor.attribute_sample_times) {
      if (item.first == "stackedOnly") {
        saw_stacked_only_samples = true;
        assert(item.second.size() == 2u);
        assert(item.second[0] == -5.0);
        assert(item.second[1] == 15.0);
      }
    }
    assert(saw_stacked_only_samples);

    const auto report = AssessEngineRuntimeSupport(*stage);
    assert(report.uses_composed_layer_stack);
    assert(report.uses_time_samples);
    assert(report.recommended_mode == EngineRuntimeMode::PreBakedAssetsOnly);
  }

  {
    std::string err;
    auto stage = Stage::OpenFromRootFile(fixture("parity_namespace.usda"), RootLayerSublayersPolicy::DepthFirst, &err);
    assert(stage && err.empty());
    const auto report = AssessEngineRuntimeSupport(*stage);
    assert(report.uses_references);
    assert(report.uses_payloads);
    assert(report.uses_relocates);
    assert(report.uses_prefix_substitutions);
    assert(report.recommended_mode == EngineRuntimeMode::PreBakedAssetsOnly);
    assert(!report.warnings.empty());
  }

  {
    std::string err;
    auto stage = Stage::OpenFromRootFile(fixture("parity_variants.usda"), RootLayerSublayersPolicy::DepthFirst, &err);
    assert(stage && err.empty());
    const auto report = AssessEngineRuntimeSupport(*stage);
    assert(report.uses_variant_selection);
    assert(report.uses_variant_sets);
    assert(report.recommended_mode == EngineRuntimeMode::PreBakedAssetsOnly);
  }

  {
    std::string err;
    auto stage =
        Stage::OpenFromRootFile(fixture("parity_skel_skinning.usda"), RootLayerSublayersPolicy::DepthFirst, &err);
    assert(stage && err.empty());
    const auto snapshot = BuildEngineSceneSnapshot(*stage, 1.0);
    assert(snapshot.skel_root_paths.size() == 1u);
    assert(snapshot.skel_root_paths[0] == Path::FromString("/World/SkelCharacter"));
    assert(snapshot.skel_animation_paths.size() == 1u);
    assert(snapshot.skel_animation_paths[0] == Path::FromString("/World/SkelCharacter/Anim"));
    assert(snapshot.skel_bound_geom_paths.size() == 1u);
    assert(snapshot.skel_bound_geom_paths[0] == Path::FromString("/World/SkelCharacter/Body"));
    const auto* body = find_node(snapshot, Path::FromString("/World/SkelCharacter/Body"));
    assert(body != nullptr);
    assert(body->has_skel_binding);
    assert(body->skel_skeleton_path == Path::FromString("/World/SkelCharacter/Skeleton"));
    assert(!body->has_blend_shapes);

    const auto report = AssessEngineRuntimeSupport(*stage);
    assert(report.uses_skel_bound_meshes);
    assert(report.uses_skel_animation);
    assert(!report.uses_blend_shapes);
    assert(report.uses_time_samples);
    assert(report.recommended_mode == EngineRuntimeMode::PreBakedAssetsOnly);
  }

  {
    std::string err;
    auto stage =
        Stage::OpenFromRootFile(fixture("parity_skel_blend_shapes.usda"), RootLayerSublayersPolicy::DepthFirst, &err);
    assert(stage && err.empty());
    const auto snapshot = BuildEngineSceneSnapshot(*stage, 1.0);
    assert(snapshot.blend_shape_geom_paths.size() == 1u);
    assert(snapshot.blend_shape_geom_paths[0] == Path::FromString("/World/Face"));
    assert(snapshot.skel_bound_geom_paths.empty());
    assert(snapshot.skel_animation_paths.empty());
    const auto* face = find_node(snapshot, Path::FromString("/World/Face"));
    assert(face != nullptr);
    assert(face->has_blend_shapes);
    assert(face->blend_shape_tokens.size() == 2u);
    assert(face->blend_shape_tokens[0] == "Smile");
    assert(face->blend_shape_tokens[1] == "Blink");

    const auto report = AssessEngineRuntimeSupport(*stage);
    assert(report.uses_blend_shapes);
    assert(!report.uses_skel_bound_meshes);
    assert(!report.uses_skel_animation);
    assert(report.uses_time_samples);
    assert(report.recommended_mode == EngineRuntimeMode::PreBakedAssetsOnly);
  }

  {
    std::string err;
    auto stage =
        Stage::OpenFromRootFile(fixture("parity_shade_preview.usda"), RootLayerSublayersPolicy::DepthFirst, &err);
    assert(stage && err.empty());
    const auto snapshot = BuildEngineSceneSnapshot(*stage, 1.0);
    assert(snapshot.material_bound_geom_paths.size() == 1u);
    assert(snapshot.material_bound_geom_paths[0] == Path::FromString("/World/Cube"));
    assert(snapshot.material_paths.size() == 1u);
    assert(snapshot.material_paths[0] == Path::FromString("/World/Looks/Material"));
    assert(snapshot.preview_surface_shader_paths.size() == 1u);
    assert(snapshot.preview_surface_shader_paths[0] == Path::FromString("/World/Looks/Material/PreviewSurface"));
    const auto* cube = find_node(snapshot, Path::FromString("/World/Cube"));
    assert(cube != nullptr);
    assert(cube->has_material_binding);
    assert(cube->material_path == Path::FromString("/World/Looks/Material"));

    const auto report = AssessEngineRuntimeSupport(*stage);
    assert(report.uses_material_bindings);
    assert(report.uses_preview_surface);
    assert(report.recommended_mode == EngineRuntimeMode::HybridMetadata);
  }

  {
    std::string err;
    auto stage = Stage::OpenFromRootFile(fixture("parity_lux_sphere.usda"), RootLayerSublayersPolicy::DepthFirst, &err);
    assert(stage && err.empty());
    const auto snapshot = BuildEngineSceneSnapshot(*stage, 1.0);
    assert(snapshot.lux_light_paths.size() == 1u);
    assert(snapshot.lux_light_paths[0] == Path::FromString("/World/Bulb"));
    const auto* bulb = find_node(snapshot, Path::FromString("/World/Bulb"));
    assert(bulb != nullptr);
    assert(bulb->has_lux_light);
    assert(bulb->lux_light_type == "SphereLight");

    const auto report = AssessEngineRuntimeSupport(*stage);
    assert(report.uses_lux_lights);
    assert(report.recommended_mode == EngineRuntimeMode::ExperimentalLiveStage);
  }

  {
    std::string err;
    auto stage =
        Stage::OpenFromRootFile(fixture("parity_shade_pbr_textures.usda"), RootLayerSublayersPolicy::DepthFirst, &err);
    assert(stage && err.empty());
    const auto snapshot = BuildEngineSceneSnapshot(*stage, 1.0);
    assert(snapshot.preview_surface_shader_paths.size() == 1u);
    assert(snapshot.preview_surface_textured_shader_paths.size() == 1u);
    assert(snapshot.preview_surface_textured_shader_paths[0] ==
           Path::FromString("/World/Looks/Material/PreviewSurface"));

    const auto report = AssessEngineRuntimeSupport(*stage);
    assert(report.uses_preview_surface);
    assert(report.uses_preview_surface_textures);
  }

  {
    std::string err;
    auto stage = Stage::OpenFromRootFile(fixture("parity_kind_active_refs.usda"), RootLayerSublayersPolicy::DepthFirst,
                                         &err);
    assert(stage && err.empty());
    const auto snapshot = BuildEngineSceneSnapshot(*stage, 1.0);
    assert(snapshot.composed_kind_prim_paths.size() >= 3u);

    const auto* ref_host = find_node(snapshot, Path::FromString("/World/RefHost"));
    const auto* payload_host = find_node(snapshot, Path::FromString("/World/PayloadHost"));
    const auto* inherit_host = find_node(snapshot, Path::FromString("/World/InheritHost"));
    assert(ref_host != nullptr && ref_host->has_prim_kind);
    assert(ref_host->prim_kind.GetText() == std::string("component"));
    assert(ref_host->has_prim_active_opinion);
    assert(!ref_host->active);
    assert(payload_host != nullptr && payload_host->prim_kind.GetText() == std::string("group"));
    assert(inherit_host != nullptr && inherit_host->prim_kind.GetText() == std::string("assembly"));

    const auto report = AssessEngineRuntimeSupport(*stage);
    assert(report.uses_composed_prim_kind);
    assert(report.uses_prim_active_opinions);
    assert(report.uses_kind_active_through_arcs);
    assert(report.uses_references);
    assert(report.uses_payloads);
    assert(report.uses_inherits);
    assert(report.recommended_mode == EngineRuntimeMode::PreBakedAssetsOnly);
  }

  {
    std::string err;
    auto stage = Stage::OpenFromRootFile(fixture("parity_custom_data_inherit.usda"),
                                         RootLayerSublayersPolicy::DepthFirst, &err);
    assert(stage && err.empty());
    const Path host_path = Path::FromString("/World/Host");
    const auto host_prim = stage->GetPrimAtPath(host_path);
    assert(host_prim.IsValid());
    assert(host_prim.HasInherits());
    const auto keys = host_prim.ListCustomDataKeys();
    assert(keys.size() == 2u);
    assert(std::find(keys.begin(), keys.end(), "role") != keys.end());
    assert(std::find(keys.begin(), keys.end(), "priority") != keys.end());
    const auto* host_node = find_node(BuildEngineSceneSnapshot(*stage, 1.0), host_path);
    assert(host_node != nullptr);
    assert(host_node->has_inherits);

    const auto report = AssessEngineRuntimeSupport(*stage);
    assert(report.uses_inherits);
    assert(report.uses_custom_data);
  }

  {
    std::string err;
    auto stage = Stage::OpenFromRootFile(fixture("parity_physics_scene.usda"), RootLayerSublayersPolicy::DepthFirst,
                                         &err);
    assert(stage && err.empty());
    const auto snapshot = BuildEngineSceneSnapshot(*stage, 1.0);
    assert(snapshot.physics_scene_paths.size() == 1u);
    assert(snapshot.physics_scene_paths[0] == Path::FromString("/World/Physics"));
    const auto* physics = find_node(snapshot, Path::FromString("/World/Physics"));
    assert(physics != nullptr && physics->has_physics_scene);

    const auto report = AssessEngineRuntimeSupport(*stage);
    assert(report.uses_physics_scenes);
    assert(report.recommended_mode == EngineRuntimeMode::ExperimentalLiveStage);
  }

  return 0;
}
