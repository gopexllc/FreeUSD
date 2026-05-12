from __future__ import annotations

from pathlib import Path

from freeusd.sdf import Path as SdfPath
from freeusd.tf import Token
from freeusd.usd import RootLayerSublayersPolicy, Stage
from freeusd.usd.crate import (
    read_usdc_path_table_from_path,
    read_usdc_string_table_from_path,
    read_usdc_token_table_from_path,
)
from freeusd.usdGeom import Boundable, Imageable
from freeusd.usdUtils import flatten_stage_at_time


FIXTURES = Path(__file__).resolve().parents[1] / "fixtures"


def test_parity_stack_fixture_drives_time_remap_and_flatten() -> None:
    stage = Stage.open_from_root_file(str(FIXTURES / "parity_stack_root.usda"), RootLayerSublayersPolicy.depth_first)
    assert stage is not None
    model = SdfPath.from_string("/World/Model")
    assert stage.read_field_double(model, Token("stackedOnly"), 15.0) == 50.0
    assert stage.read_field_double(model, Token("stackedOnly"), -5.0) == 50.0
    assert stage.list_composed_field_sample_times(model, Token("stackedOnly")) == [-5.0, 15.0]

    flat = flatten_stage_at_time(stage, 15.0)
    assert flat.list_sample_times(model, Token("stackedOnly")) == [-5.0, 15.0]
    assert flat.get_field(model, Token("stackedOnly")).as_double() == 50.0


def test_parity_namespace_fixture_executes_references_and_payloads() -> None:
    stage = Stage.open_from_root_file(str(FIXTURES / "parity_namespace.usda"), RootLayerSublayersPolicy.depth_first)
    assert stage is not None
    published = SdfPath.from_string("/Library/Published")
    ref_leaf = SdfPath.from_string("/Library/Published/RefLeaf")
    payload_leaf = SdfPath.from_string("/Library/Published/PayloadLeaf")

    assert stage.prim_at(ref_leaf).is_valid()
    assert stage.prim_at(payload_leaf).is_valid()
    assert stage.read_field_double(published, Token("refOnly"), 1.0) == 11.0
    assert stage.read_field_double(ref_leaf, Token("branch"), 1.0) == 22.0
    assert stage.read_field_double(published, Token("payloadOnly"), 1.0) == 33.0
    assert stage.read_field_double(payload_leaf, Token("branch"), 1.0) == 44.0


def test_parity_variant_fixture_executes_selected_variant_payload() -> None:
    stage = Stage.open_from_root_file(str(FIXTURES / "parity_variants.usda"), RootLayerSublayersPolicy.depth_first)
    assert stage is not None
    host = SdfPath.from_string("/World/VariantHost")
    child = SdfPath.from_string("/World/VariantHost/VariantChild")
    assert stage.prim_at(child).is_valid()
    assert stage.read_field_double(host, Token("variantValue"), 1.0) == 5.0
    assert stage.read_field_double(child, Token("branch"), 1.0) == 9.0


def test_parity_tables_fixture_decodes_structured_usdc_tables() -> None:
    usdc = str(FIXTURES / "parity_tables.usdc")
    ok, tokens, err = read_usdc_token_table_from_path(usdc, 8, 1024)
    assert ok and err == ""
    assert tokens == ["render", "invisible"]

    ok, strings, err = read_usdc_string_table_from_path(usdc, 8, 1024)
    assert ok and err == ""
    assert strings == ["hello", "world"]

    ok, paths, err = read_usdc_path_table_from_path(usdc, 8, 1024)
    assert ok and err == ""
    assert [p.text() for p in paths] == ["/World", "/World/Cube"]


def test_parity_imageable_fixture_is_primary_scene_anchor() -> None:
    stage = Stage.open_from_root_file(str(FIXTURES / "parity_imageable.usda"), RootLayerSublayersPolicy.depth_first)
    assert stage is not None
    cube_prim = stage.prim_at(SdfPath.from_string("/World/Cube"))
    cube = Imageable(cube_prim)
    assert cube.compute_purpose(1.0) == "render"
    assert cube.compute_visibility(1.0) is False
    world = Boundable(cube_prim).compute_world_bound(1.0)
    assert world.min.as_array() == [0.0, 1.0, 2.0]
    assert world.max.as_array() == [2.0, 3.0, 4.0]
