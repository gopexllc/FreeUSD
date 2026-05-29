from __future__ import annotations

from pathlib import Path

from freeusd.sdf import Path as SdfPath
from freeusd.usd import RootLayerSublayersPolicy, Stage
from freeusd.usdUtils import (
    EngineRuntimeMode,
    assess_engine_runtime_support,
    build_engine_prim_editor_view,
    build_engine_scene_snapshot,
    engine_runtime_mode_name,
)


FIXTURES = Path(__file__).resolve().parents[1] / "fixtures"


def _open_stage(name: str) -> Stage:
    stage = Stage.open_from_root_file(str(FIXTURES / name), RootLayerSublayersPolicy.depth_first)
    assert stage is not None
    return stage


def test_engine_scene_snapshot_and_runtime_mode_bindings() -> None:
    stage = _open_stage("parity_imageable.usda")
    snapshot = build_engine_scene_snapshot(stage, 1.0)

    assert snapshot.default_prim_name == "World"
    assert "parity_imageable.usda" in snapshot.root_identifier
    assert snapshot.pseudo_root_path == SdfPath.absolute_root()

    nodes_by_path = {node.path.text(): node for node in snapshot.nodes}
    world = nodes_by_path["/World"]
    cube = nodes_by_path["/World/Cube"]

    assert [path.text() for path in world.child_paths] == ["/World/Cube"]
    assert cube.purpose == "render"
    assert cube.visible is False
    assert cube.world_bound.min.as_array() == [0.0, 1.0, 2.0]
    assert cube.world_bound.max.as_array() == [2.0, 3.0, 4.0]

    report = assess_engine_runtime_support(stage)
    assert report.recommended_mode == EngineRuntimeMode.experimental_live_stage
    assert engine_runtime_mode_name(report.recommended_mode) == "experimental_live_stage"


def test_engine_editor_view_and_prebake_reports_bindings() -> None:
    stack_stage = _open_stage("parity_stack_root.usda")
    editor = build_engine_prim_editor_view(stack_stage.prim_at(SdfPath.from_string("/World/Model")), 15.0)

    assert editor.path == SdfPath.from_string("/World/Model")
    assert "stackedOnly" in editor.composed_field_names
    sample_times = {name: times for name, times in editor.attribute_sample_times}
    assert sample_times["stackedOnly"] == [-5.0, 15.0]

    stack_report = assess_engine_runtime_support(stack_stage)
    assert stack_report.uses_composed_layer_stack
    assert stack_report.uses_time_samples
    assert stack_report.recommended_mode == EngineRuntimeMode.pre_baked_assets_only

    namespace_report = assess_engine_runtime_support(_open_stage("parity_namespace.usda"))
    assert namespace_report.uses_references
    assert namespace_report.uses_payloads
    assert namespace_report.uses_relocates
    assert namespace_report.uses_prefix_substitutions

    variant_report = assess_engine_runtime_support(_open_stage("parity_variants.usda"))
    assert variant_report.uses_variant_selection
    assert variant_report.uses_variant_sets
    assert variant_report.recommended_mode == EngineRuntimeMode.pre_baked_assets_only

    shade_report = assess_engine_runtime_support(_open_stage("parity_shade_preview.usda"))
    assert shade_report.uses_material_bindings
    assert shade_report.uses_preview_surface

    lux_stage = _open_stage("parity_lux_sphere.usda")
    lux_snapshot = build_engine_scene_snapshot(lux_stage, 1.0)
    assert [path.text() for path in lux_snapshot.lux_light_paths] == ["/World/Bulb"]
    lux_report = assess_engine_runtime_support(lux_stage)
    assert lux_report.uses_lux_lights

    pbr_stage = _open_stage("parity_shade_pbr_textures.usda")
    pbr_snapshot = build_engine_scene_snapshot(pbr_stage, 1.0)
    assert [path.text() for path in pbr_snapshot.preview_surface_textured_shader_paths] == [
        "/World/Looks/Material/PreviewSurface"
    ]
    pbr_report = assess_engine_runtime_support(pbr_stage)
    assert pbr_report.uses_preview_surface_textures

    kind_stage = _open_stage("parity_kind_active_refs.usda")
    kind_snapshot = build_engine_scene_snapshot(kind_stage, 1.0)
    assert len(kind_snapshot.composed_kind_prim_paths) >= 3
    kind_report = assess_engine_runtime_support(kind_stage)
    assert kind_report.uses_kind_active_through_arcs
    assert kind_report.uses_composed_prim_kind
    assert kind_report.recommended_mode == EngineRuntimeMode.pre_baked_assets_only

    spec_kind_stage = _open_stage("parity_kind_active_specializes.usda")
    spec_host = spec_kind_stage.prim_at(SdfPath.from_string("/World/SpecHost"))
    assert spec_host.has_specializes()
    assert spec_host.get_prim_kind().text() == "assembly"
    assert not spec_host.is_active()
    spec_kind_report = assess_engine_runtime_support(spec_kind_stage)
    assert spec_kind_report.uses_specializes
    assert spec_kind_report.uses_kind_active_through_arcs
    assert spec_kind_report.recommended_mode == EngineRuntimeMode.pre_baked_assets_only

    specializes_stage = _open_stage("parity_specializes.usda")
    specializes_report = assess_engine_runtime_support(specializes_stage)
    assert specializes_report.uses_specializes
    assert specializes_report.recommended_mode == EngineRuntimeMode.pre_baked_assets_only

    custom_stage = _open_stage("parity_custom_data_inherit.usda")
    host = next(node for node in build_engine_scene_snapshot(custom_stage, 1.0).nodes if node.path.text() == "/World/Host")
    assert "role" in host.custom_data_keys
    custom_report = assess_engine_runtime_support(custom_stage)
    assert custom_report.uses_inherits
    assert custom_report.uses_custom_data
    assert custom_report.uses_custom_data_through_arcs

    refs_stage = _open_stage("parity_custom_data_refs.usda")
    refs_report = assess_engine_runtime_support(refs_stage)
    assert refs_report.uses_references
    assert refs_report.uses_payloads
    assert refs_report.uses_custom_data
    assert refs_report.uses_custom_data_through_arcs
    assert refs_report.recommended_mode == EngineRuntimeMode.pre_baked_assets_only

    physics_stage = _open_stage("parity_physics_scene.usda")
    physics_snapshot = build_engine_scene_snapshot(physics_stage, 1.0)
    assert [path.text() for path in physics_snapshot.physics_scene_paths] == ["/World/Physics"]
    physics_report = assess_engine_runtime_support(physics_stage)
    assert physics_report.uses_physics_scenes

    rigid_body_stage = _open_stage("parity_physics_rigid_body.usda")
    rigid_body_snapshot = build_engine_scene_snapshot(rigid_body_stage, 1.0)
    assert [path.text() for path in rigid_body_snapshot.rigid_body_api_paths] == ["/World/Body"]
    body = next(node for node in rigid_body_snapshot.nodes if node.path.text() == "/World/Body")
    assert body.has_rigid_body_api
    rigid_body_report = assess_engine_runtime_support(rigid_body_stage)
    assert rigid_body_report.uses_rigid_body_api

    collision_stage = _open_stage("parity_physics_collision.usda")
    collision_snapshot = build_engine_scene_snapshot(collision_stage, 1.0)
    assert [path.text() for path in collision_snapshot.collision_api_paths] == ["/World/Collider"]
    collider = next(node for node in collision_snapshot.nodes if node.path.text() == "/World/Collider")
    assert collider.has_collision_api
    collision_report = assess_engine_runtime_support(collision_stage)
    assert collision_report.uses_collision_api

    collision_inherit_stage = _open_stage("parity_physics_collision_inherit.usda")
    collision_inherit_snapshot = build_engine_scene_snapshot(collision_inherit_stage, 1.0)
    assert [path.text() for path in collision_inherit_snapshot.collision_api_paths] == ["/World/Collider"]
    assert assess_engine_runtime_support(collision_inherit_stage).uses_collision_api

    joint_stage = _open_stage("parity_physics_fixed_joint.usda")
    joint_snapshot = build_engine_scene_snapshot(joint_stage, 1.0)
    assert [path.text() for path in joint_snapshot.physics_fixed_joint_paths] == ["/World/Anchor"]
    anchor = next(node for node in joint_snapshot.nodes if node.path.text() == "/World/Anchor")
    assert anchor.has_physics_fixed_joint
    assert anchor.physics_fixed_joint_body0.text() == "/World/BodyA"
    assert anchor.physics_fixed_joint_body1.text() == "/World/BodyB"
    joint_report = assess_engine_runtime_support(joint_stage)
    assert joint_report.uses_physics_fixed_joints
    assert joint_report.uses_relationships

    vol_stage = _open_stage("parity_vol_openvdb.usda")
    vol_snapshot = build_engine_scene_snapshot(vol_stage, 1.0)
    assert [path.text() for path in vol_snapshot.open_vdb_asset_paths] == ["/World/Smoke"]
    smoke = next(node for node in vol_snapshot.nodes if node.path.text() == "/World/Smoke")
    assert smoke.has_open_vdb_asset
    assert smoke.open_vdb_file_path == "volumes/smoke.vdb"
    assert smoke.open_vdb_field_name == "density"
    vol_report = assess_engine_runtime_support(vol_stage)
    assert vol_report.uses_open_vdb_assets

    volume_stage = _open_stage("parity_vol_volume.usda")
    volume_snapshot = build_engine_scene_snapshot(volume_stage, 1.0)
    assert [path.text() for path in volume_snapshot.volume_paths] == ["/World/Cloud"]
    assert [path.text() for path in volume_snapshot.open_vdb_asset_paths] == ["/World/Cloud/Smoke"]
    cloud = next(node for node in volume_snapshot.nodes if node.path.text() == "/World/Cloud")
    assert cloud.has_volume
    assert [path.text() for path in cloud.volume_field_asset_paths] == ["/World/Cloud/Smoke"]
    volume_report = assess_engine_runtime_support(volume_stage)
    assert volume_report.uses_volumes
    assert volume_report.uses_open_vdb_assets
