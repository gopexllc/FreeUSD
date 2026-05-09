from __future__ import annotations

import freeusd
from freeusd.sdf import Layer, Path
from freeusd.tf import Token
from freeusd.usd import Stage
from freeusd.vt import Value


def test_stage_prim_roundtrip() -> None:
    layer = Layer.new_anonymous("test")
    cube = Path.from_string("/World/Cube")
    layer.set_field(cube, Token("size"), Value.make_double(3.0))

    stage = Stage.attach_root_layer(layer)
    assert stage is not None

    prim = stage.prim_at(cube)
    assert prim.is_valid()
    assert prim.has_attribute(Token("size"))
    v = prim.get_attribute(Token("size"))
    assert not v.is_empty()
    assert v.as_double() == 3.0


def test_path_parse() -> None:
    p = Path.from_string("/World/Cube.xformOp:translate")
    assert p.is_property_path()
    assert p.prim_path().text() == "/World/Cube"


def test_version_export() -> None:
    assert freeusd.version()
    assert freeusd.version_tuple()[0] >= 0
