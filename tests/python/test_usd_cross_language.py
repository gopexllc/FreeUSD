from __future__ import annotations

from pathlib import Path

import freeusd.io as io
from freeusd.gf import Matrix4d, Quatd, Quatf
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


def test_usd_cross_language_field_read_contract() -> None:
    text = FIXTURE.read_text(encoding="utf-8")
    layer = Layer.new_anonymous("cross_lang_field_reads")
    assert io.load_from_string(text, layer).ok

    stage = Stage.attach_root_layer(layer)
    assert stage is not None

    child = SdfPath.from_string("/Scene/Child")
    partial = SdfPath.from_string("/Scene/Partial")
    missing = SdfPath.from_string("/Scene/Missing")
    prim = stage.prim_at(child)
    assert prim.is_valid()
    assert stage.prim_at(missing).is_valid() is False

    assert stage.has_field_opinion(child, Token("mass"))
    assert not stage.has_field_opinion(child, Token("missingAttr"))
    assert prim.has_attribute(Token("mass"))
    assert not prim.has_attribute(Token("missingAttr"))

    assert stage.read_field_double(child, Token("mass"), 1.0) == 2.5
    assert stage.read_field_double(child, Token("mass"), 2.0) == 4.0
    assert prim.read_field_double(Token("mass"), 2.0) == 4.0

    assert stage.read_field_float(child, Token("density"), 1.0) == 1.25
    assert stage.read_field_float(child, Token("mass"), 1.0) == 2.5
    assert prim.read_field_float(Token("mass"), 1.0) == 2.5

    assert stage.read_field_bool(child, Token("enabled"), 1.0) is True
    assert stage.read_field_int64(child, Token("count"), 1.0) == -7
    assert stage.read_field_int64(child, Token("mass"), 1.0) == 2
    assert stage.read_field_string(child, Token("label"), 1.0) == "hello"
    assert stage.read_field_string(child, Token("kind"), 1.0) == "component"

    assert stage.read_field_vec3d(child, Token("extent"), 1.0) == (1.0, 2.0, 3.0)
    assert stage.read_field_vec3f(child, Token("displayColor"), 1.0) == (0.25, 0.5, 0.75)
    assert stage.read_field_vec3f(child, Token("extent"), 1.0) == (1.0, 2.0, 3.0)
    assert stage.read_field_matrix4d(child, Token("xf"), 1.0) == Matrix4d.Identity()
    assert stage.read_field_quatd(child, Token("qd"), 1.0) == Quatd.Identity()
    assert stage.read_field_quatf(child, Token("qf"), 1.0) == Quatf(0.70710677, 0.0, 0.0, 0.70710677)
    assert stage.read_field_quatf(child, Token("qd"), 1.0) == Quatf(1.0, 0.0, 0.0, 0.0)

    kind = stage.read_field_token(child, Token("kind"), 1.0)
    assert kind is not None and kind.text() == "component"
    tags = stage.read_field_token_array(child, Token("tags"), 1.0)
    assert tags is not None and [t.text() for t in tags] == ["a", "b"]

    assert stage.read_field_at_time(child, Token("missingAttr"), 1.0) is None
    assert stage.read_field_double(child, Token("missingAttr"), 1.0) is None
    assert stage.read_field_double(missing, Token("mass"), 1.0) is None
    assert stage.read_field_double(partial, Token("missingAttr"), 1.0) is None
    assert prim.read_field_at_time(Token("missingAttr"), 1.0) is None
    assert prim.read_field_double(Token("missingAttr"), 1.0) is None

    assert stage.read_field_double(child, Token("label"), 1.0) is None
    assert stage.read_field_bool(child, Token("label"), 1.0) is None
    assert stage.read_field_vec3d(child, Token("displayColor"), 1.0) is None
    assert stage.read_field_matrix4d(child, Token("kind"), 1.0) is None
    assert stage.read_field_quatd(child, Token("qf"), 1.0) is None
    assert stage.read_field_token(child, Token("label"), 1.0) is None
    assert stage.read_field_token_array(child, Token("kind"), 1.0) is None


def test_usd_cross_language_composition_helpers() -> None:
    text = FIXTURE.read_text(encoding="utf-8")
    layer = Layer.new_anonymous("cross_lang_composition")
    assert io.load_from_string(text, layer).ok

    stage = Stage.attach_root_layer(layer)
    assert stage is not None

    child = SdfPath.from_string("/Scene/Child")
    arc_host = SdfPath.from_string("/Scene/ArcHost")

    assert stage.list_composed_field_sample_times(child, Token("mass")) == [2.0]
    assert stage.get_composed_prim_custom_data(child, "tag").as_int32() == 99
    assert stage.prim_custom_data_key_in_any_layer(child, "tag")
    assert stage.list_composed_prim_custom_data_keys(child) == ["tag"]

    assert stage.has_prim_inherits(arc_host)
    assert stage.read_prim_inherits(arc_host) == [SdfPath.from_string("/Scene/Child")]
    assert not stage.has_prim_inherits(child)

    assert stage.get_composed_prim_custom_data(arc_host, "tag").as_int32() == 99
    assert stage.prim_custom_data_key_in_any_layer(arc_host, "tag")
    assert stage.prim_at(arc_host).has_custom_data_key("tag")
    assert "tag" in stage.list_composed_prim_custom_data_keys(arc_host)
