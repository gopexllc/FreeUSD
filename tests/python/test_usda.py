from __future__ import annotations

import freeusd.io as io
import freeusd.usd as usd
from freeusd.sdf import Layer, Path, PrimSpecifierKind
from freeusd.tf import Token
from freeusd.vt import Value


def test_time_samples_roundtrip() -> None:
    src = """#usda 1.0
(
    doc = "ts"
)

def "Prim"
{
    float x = 0
    float x.timeSamples = {
        1: 1.5,
        2: 2.5,
    }
}
"""
    layer = Layer.new_anonymous("ts")
    assert io.load_from_string(src, layer).ok
    p = Path.from_string("/Prim")
    times = layer.list_sample_times(p, Token("x"))
    assert times == [1.0, 2.0]
    v = layer.get_field_at_time(p, Token("x"), 1.5)
    assert v is not None
    assert v.as_double() == 1.5


def test_usda_roundtrip_python() -> None:
    src = """#usda 1.0
(
    doc = "py"
)

def "Hello"
{
    int count = 7
}
"""
    layer = Layer.new_anonymous("t")
    r = io.load_from_string(src, layer)
    assert r.ok, (r.line, r.message)

    p = Path.from_string("/Hello")
    assert layer.has_field(p, Token("count"))
    v = layer.get_field(p, Token("count"))
    assert v.as_double() == 7.0

    out = io.save_to_string(layer)
    layer2 = Layer.new_anonymous("t2")
    r2 = io.load_from_string(out, layer2)
    assert r2.ok
    v2 = layer2.get_field(p, Token("count"))
    assert v2.as_double() == 7.0


def test_layer_metadata_and_prim_composition_bindings() -> None:
    src = """#usda 1.0
(
    doc = "Composition test"
    defaultPrim = Hello
    subLayers = [
        @./base.usda@,
        @schemas.usda@,
    ]
)

class Xform "Inactive"
(
    prepend references = @./ref.usda@</Prim>
    active = false
    doc = "class prim"
)
{
}

over "Hello"
(
    doc = "over node"
)
{
}

def Xform "Hello"
(
    doc = "def node"
)
{
}
"""
    layer = Layer.new_anonymous("comp")
    r = io.load_from_string(src, layer)
    assert r.ok, (r.line, r.message)

    assert layer.documentation() == "Composition test"
    assert layer.default_prim() == "Hello"
    assert layer.sub_layers() == ["./base.usda", "schemas.usda"]

    inactive = Path.from_string("/Inactive")
    hello = Path.from_string("/Hello")
    refs = layer.list_references(inactive)
    assert len(refs) == 1 and "ref.usda" in refs[0]

    layer.set_prim_active(inactive, True)
    layer.add_reference(hello, "/tmp/extra.usda")
    layer.set_prim_specifier(hello, PrimSpecifierKind.class_)

    out = io.save_to_string(layer)
    layer2 = Layer.new_anonymous("comp2")
    assert io.load_from_string(out, layer2).ok
    hello2 = Path.from_string("/Hello")
    assert any("extra.usda" in p for p in layer2.list_references(hello2))
    assert layer2.get_prim_specifier(hello2) == PrimSpecifierKind.class_

    layer.clear()
    assert layer.documentation() == ""
    assert layer.default_prim() is None
    assert layer.sub_layers() == []


def test_usda_relationships_roundtrip_and_prim() -> None:
    src = """#usda 1.0
(
    doc = \"rel\"
)

def Xform \"W\"
{
    def \"C\"
    {
        rel scene:shot = [</Shots/A>, </Shots/B>]
        rel mat:binding = </Materials/M>
        prepend rel proxyPrim = </Proxy/P>
    }
}
"""
    layer = Layer.new_anonymous("r")
    assert io.load_from_string(src, layer).ok
    c = Path.from_string("/W/C")
    mats = layer.list_relationship_names(c)
    assert "scene:shot" in mats
    assert "mat:binding" in mats
    t1 = layer.get_relationship_targets(c, Token("scene:shot"))
    assert len(t1) == 2
    assert t1[0].text() == "/Shots/A"

    st = usd.Stage.attach_root_layer(layer)
    prim_c = st.prim_at(c)
    assert prim_c.has_relationship(Token("proxyPrim"))
    proxies = prim_c.get_relationship_targets(Token("proxyPrim"))
    assert proxies[0].text() == "/Proxy/P"

    out = io.save_to_string(layer)
    layer2 = Layer.new_anonymous("r2")
    assert io.load_from_string(out, layer2).ok
    t2 = layer2.get_relationship_targets(Path.from_string("/W/C"), Token("scene:shot"))
    assert len(t2) == 2
