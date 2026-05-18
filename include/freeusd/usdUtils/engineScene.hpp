#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "freeusd/export.hpp"
#include "freeusd/gf/bbox3d.hpp"
#include "freeusd/gf/matrix4d.hpp"
#include "freeusd/sdf/path.hpp"
#include "freeusd/tf/token.hpp"
#include "freeusd/usd/prim.hpp"
#include "freeusd/usd/stage.hpp"

namespace freeusd::usdUtils {

/// Recommended engine runtime integration mode for the currently validated subset.
enum class EngineRuntimeMode {
  /// Bake authored USD content into engine-native assets and avoid live stage evaluation in shipping runtime.
  PreBakedAssetsOnly = 0,
  /// Allow runtime reads of a narrow composed metadata subset while geometry stays pre-baked.
  HybridMetadata,
  /// Live composed stage evaluation is possible for the narrowest static scenes, but remains the riskiest path.
  ExperimentalLiveStage,
};

FREEUSD_API std::string_view EngineRuntimeModeName(EngineRuntimeMode mode) noexcept;

/// One prim record exported for engine import and editor scene trees.
struct FREEUSD_API EngineSceneNode {
  freeusd::sdf::Path path;
  std::string name;
  freeusd::tf::Token prim_kind;
  bool active{true};
  bool visible{true};
  bool has_references{false};
  bool has_payloads{false};
  bool has_inherits{false};
  bool has_specializes{false};
  bool has_variant_selections{false};
  bool has_variant_sets{false};
  bool has_time_samples{false};
  freeusd::gf::Matrix4d local_transform{freeusd::gf::Matrix4d::Identity()};
  freeusd::gf::Matrix4d local_to_world_transform{freeusd::gf::Matrix4d::Identity()};
  freeusd::gf::BBox3d local_bound{freeusd::gf::BBox3d::Empty()};
  freeusd::gf::BBox3d world_bound{freeusd::gf::BBox3d::Empty()};
  std::string purpose{"default"};
  std::vector<std::string> composed_field_names;
  std::vector<std::string> composed_relationship_names;
  std::vector<std::string> custom_data_keys;
  std::vector<std::string> variant_selection_sets;
  std::vector<std::string> variant_set_names;
  std::vector<freeusd::sdf::Path> child_paths;
  /// True when ``skel:skeleton`` resolves for this geom prim.
  bool has_skel_binding{false};
  freeusd::sdf::Path skel_skeleton_path{};
  /// True when ``skel:blendShapes`` and ``skel:blendShapeTargets`` are authored.
  bool has_blend_shapes{false};
  std::vector<std::string> blend_shape_tokens;
  /// True when ``material:binding`` resolves to at least one target.
  bool has_material_binding{false};
  freeusd::sdf::Path material_path{};
  /// True when the prim type name matches a supported ``usdLux`` light family.
  bool has_lux_light{false};
  /// OpenUSD-shaped lux type name (for example ``SphereLight``); empty when @ref has_lux_light is false.
  std::string lux_light_type;
  /// True when a bound or local ``UsdPreviewSurface`` resolves at least one texture asset path.
  bool has_preview_surface_textures{false};
};

/// Stage-level snapshot for USDA-first import and engine/editor cache generation.
struct FREEUSD_API EngineSceneSnapshot {
  std::string root_identifier;
  freeusd::sdf::Path pseudo_root_path{freeusd::sdf::Path::AbsoluteRootPath()};
  std::string default_prim_name;
  std::optional<double> start_time_code;
  std::optional<double> end_time_code;
  std::optional<double> time_codes_per_second;
  std::optional<double> frames_per_second;
  std::optional<int> frame_precision;
  std::optional<double> meters_per_unit;
  std::optional<std::string> up_axis;
  std::vector<freeusd::sdf::Path> prim_order;
  std::vector<EngineSceneNode> nodes;
  /// Geom prims with a resolved ``skel:skeleton`` binding.
  std::vector<freeusd::sdf::Path> skel_bound_geom_paths;
  /// Geom prims with authored blend-shape bindings.
  std::vector<freeusd::sdf::Path> blend_shape_geom_paths;
  /// ``SkelRoot`` prims discovered during traversal.
  std::vector<freeusd::sdf::Path> skel_root_paths;
  /// ``SkelAnimation`` prims discovered during traversal.
  std::vector<freeusd::sdf::Path> skel_animation_paths;
  /// Geom prims with a resolved ``material:binding``.
  std::vector<freeusd::sdf::Path> material_bound_geom_paths;
  /// ``Material`` prims whose surface shader is ``UsdPreviewSurface``.
  std::vector<freeusd::sdf::Path> material_paths;
  /// Shader prims with ``info:id`` ``UsdPreviewSurface``.
  std::vector<freeusd::sdf::Path> preview_surface_shader_paths;
  /// ``UsdPreviewSurface`` shaders with at least one resolved texture asset path.
  std::vector<freeusd::sdf::Path> preview_surface_textured_shader_paths;
  /// Supported ``usdLux`` light prims (distant, sphere, rect, disk, cylinder, dome).
  std::vector<freeusd::sdf::Path> lux_light_paths;
};

/// Editor-facing inspection view for one prim in the validated subset.
struct FREEUSD_API EnginePrimEditorView {
  freeusd::sdf::Path path;
  std::vector<std::string> composed_field_names;
  std::vector<std::pair<std::string, std::vector<double>>> attribute_sample_times;
  std::vector<std::string> composed_relationship_names;
  std::vector<std::pair<std::string, std::vector<freeusd::sdf::Path>>> relationship_targets;
  std::vector<std::string> custom_data_keys;
  std::vector<std::string> variant_selection_sets;
  std::vector<std::string> variant_set_names;
};

/// Runtime-scope report for choosing between baked assets, hybrid metadata, and experimental live stages.
struct FREEUSD_API EngineRuntimeSupportReport {
  EngineRuntimeMode recommended_mode{EngineRuntimeMode::PreBakedAssetsOnly};
  bool uses_composed_layer_stack{false};
  bool uses_references{false};
  bool uses_payloads{false};
  bool uses_inherits{false};
  bool uses_specializes{false};
  bool uses_variant_selection{false};
  bool uses_variant_sets{false};
  bool uses_relocates{false};
  bool uses_prefix_substitutions{false};
  bool uses_time_samples{false};
  bool uses_relationships{false};
  bool uses_custom_data{false};
  bool uses_attribute_connections{false};
  bool uses_skel_bound_meshes{false};
  bool uses_blend_shapes{false};
  bool uses_skel_animation{false};
  bool uses_material_bindings{false};
  bool uses_preview_surface{false};
  bool uses_preview_surface_textures{false};
  bool uses_lux_lights{false};
  std::vector<std::string> warnings;
};

/// Build one engine-oriented snapshot from the currently validated `Stage` / `usdGeom` subset.
FREEUSD_API EngineSceneSnapshot BuildEngineSceneSnapshot(const freeusd::usd::Stage& stage, double time = 1.0);

/// Inspect one prim for editor-facing tree/details panels without exposing unstable internals.
FREEUSD_API EnginePrimEditorView BuildEnginePrimEditorView(const freeusd::usd::Prim& prim, double time = 1.0);

/// Recommend the narrowest runtime mode that safely fits the scene's currently observed features.
FREEUSD_API EngineRuntimeSupportReport AssessEngineRuntimeSupport(const freeusd::usd::Stage& stage);

}  // namespace freeusd::usdUtils
