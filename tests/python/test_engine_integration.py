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
