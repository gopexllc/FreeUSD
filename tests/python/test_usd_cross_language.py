from __future__ import annotations

from pathlib import Path

import freeusd.io as io
from freeusd.sdf import Layer, Path as SdfPath
from freeusd.tf import Token
from freeusd.usd import Stage

FIXTURE = Path(__file__).resolve().parent.parent / "fixtures" / "usd_cross_language.usda"


def test_usd_cross_language_fixture_matches_cpp_contract() -> None:
    """Same on-disk USDA as C++ `usd_cross_language_test` and C `c_usd_cross_language`."""
    text = FIXTURE.read_text(encoding="utf-8")
    layer = Layer.new_anonymous("cross_lang_fixture")
    assert io.load_from_string(text, layer).ok

    assert layer.documentation() == "cross_lang_fixture"
    assert layer.default_prim() == "Scene"
    assert layer.meters_per_unit() == 0.01
    assert layer.up_axis() == "Y"
    assert layer.start_time_code() == 0.0
    assert layer.end_time_code() == 100.0

    stage = Stage.attach_root_layer(layer)
    assert stage is not None
    assert stage.has_default_prim()
    assert stage.default_prim_name() == "Scene"
    assert stage.meters_per_unit() == 0.01
    assert stage.up_axis() == "Y"
    assert stage.start_time_code() == 0.0
    assert stage.end_time_code() == 100.0

    child = SdfPath.from_string("/Scene/Child")
    prim = stage.prim_at(child)
    assert prim.is_valid()
    assert prim.name() == "Child"

    tag = stage.get_composed_prim_custom_data(child, "tag")
    assert tag is not None
    assert tag.as_int32() == 99

    assert prim.get_attribute(Token("mass"), 1.0).as_double() == 2.5
    assert prim.get_attribute(Token("mass"), 2.0).as_double() == 4.0
