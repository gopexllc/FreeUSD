from __future__ import annotations

import freeusd.io as io
import freeusd.usd as usd
from freeusd.sdf import Layer, Path, PrimSpecifierKind
from freeusd.tf import Token
from freeusd.vt import Value


def test_float3_vec3f_usda() -> None:
    src = """#usda 1.0
(
)
def "P"
{
    float3 extent = (1.5, 2.5, 3.5)
}
"""
    layer = Layer.new_anonymous("f3")
    assert io.load_from_string(src, layer).ok
    p = Path.from_string("/P")
    v = layer.get_field(p, Token("extent"))
    assert v is not None
    assert v.holds_vec3f()
    u = v.as_vec3f()
    assert abs(u.x() - 1.5) < 1e-5 and abs(u.y() - 2.5) < 1e-5 and abs(u.z() - 3.5) < 1e-5
    out = io.save_to_string(layer)
    assert "float3 extent" in out.replace("\n", " ")


def test_float_half_scalar_usda_stores_vt_float() -> None:
    """Typed ``float`` / ``half`` attributes keep ``vt::Value`` as float (not double)."""
    src = """#usda 1.0
(
)
def "P"
{
    float a = 1.5
    half b = 2.25
    float c = 3
}
"""
    layer = Layer.new_anonymous("fsc")
    assert io.load_from_string(src, layer).ok
    p = Path.from_string("/P")
    va = layer.get_field(p, Token("a"))
    vb = layer.get_field(p, Token("b"))
    vc = layer.get_field(p, Token("c"))
    assert va is not None and va.holds_float()
    assert vb is not None and vb.holds_float()
    assert vc is not None and vc.holds_float()
    assert abs(va.as_float() - 1.5) < 1e-5
    assert abs(vb.as_float() - 2.25) < 1e-5
    assert abs(vc.as_float() - 3.0) < 1e-5
    out = io.save_to_string(layer)
    joined = out.replace("\n", " ")
    assert "float a" in joined and "float c" in joined
    # Authored ``half`` is stored as float; USDA save uses ``float`` for float payloads.
    assert "float b" in joined


def test_point3f_normal3f_color3f_alias_vec3f_usda() -> None:
    """USDA ``point3f`` / ``normal3f`` / ``color3f`` parse like ``float3`` (Vec3f)."""
    src = """#usda 1.0
(
)
def "P"
{
    point3f p = (1, 2, 3)
    normal3f n = (0, 0, 1)
    color3f c = (0.1, 0.2, 0.3)
}
"""
    layer = Layer.new_anonymous("f3alias")
    assert io.load_from_string(src, layer).ok
    base = Path.from_string("/P")
    vp = layer.get_field(base, Token("p"))
    vn = layer.get_field(base, Token("n"))
    vc = layer.get_field(base, Token("c"))
    assert vp is not None and vp.holds_vec3f()
    assert vn is not None and vn.holds_vec3f()
    assert vc is not None and vc.holds_vec3f()
    assert abs(vp.as_vec3f().x() - 1.0) < 1e-5
    assert abs(vn.as_vec3f().z() - 1.0) < 1e-5
    assert abs(vc.as_vec3f().y() - 0.2) < 1e-5


def test_quatf_orient_usda_roundtrip() -> None:
    """``quatf`` tuple literals use the attribute type (distinct from ``quatd`` / ``double3``)."""
    from freeusd.gf import Matrix4d
    from freeusd.usd import Stage
    from freeusd.usdGeom import Xformable

    src = """#usda 1.0
(
)
def "Q"
{
    quatf xformOp:orient = (0.70710677, 0, 0, 0.70710677)
}
"""
    layer = Layer.new_anonymous("qf")
    assert io.load_from_string(src, layer).ok
    p = Path.from_string("/Q")
    v = layer.get_field(p, Token("xformOp:orient"))
    assert v is not None
    assert v.holds_quatf()
    q = v.as_quatf()
    assert abs(q.real - 0.70710677) < 1e-5
    assert abs(q.k - 0.70710677) < 1e-5
    stage = Stage.attach_root_layer(layer)
    xf = Xformable(stage.prim_at(p))
    m = xf.compute_local_transform(1.0)
    exp = Matrix4d.RotateDegreesZ(90.0)
    for a, b in zip(m.as_list(), exp.as_list()):
        assert abs(a - b) < 1e-4
    out = io.save_to_string(layer)
    assert "quatf" in out and "xformOp:orient" in out


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
    assert abs(v.as_double() - 2.0) < 1e-6


def test_color3f_time_samples_vec3f_usda() -> None:
    """``color3f`` in ``.timeSamples`` uses the same vec3f tuple parsing as ``float3``."""
    src = """#usda 1.0
(
)
def "Prim"
{
    color3f c = (0, 0, 0)
    color3f c.timeSamples = {
        1: (1, 0, 0),
        2: (0, 1, 0),
    }
}
"""
    layer = Layer.new_anonymous("c3ts")
    assert io.load_from_string(src, layer).ok
    p = Path.from_string("/Prim")
    times = layer.list_sample_times(p, Token("c"))
    assert times == [1.0, 2.0]
    v1 = layer.get_field_at_time(p, Token("c"), 1.0)
    v2 = layer.get_field_at_time(p, Token("c"), 2.0)
    assert v1 is not None and v1.holds_vec3f()
    assert v2 is not None and v2.holds_vec3f()
    u1 = v1.as_vec3f()
    u2 = v2.as_vec3f()
    assert abs(u1.x() - 1.0) < 1e-5 and abs(u1.y()) < 1e-5
    assert abs(u2.y() - 1.0) < 1e-5 and abs(u2.x()) < 1e-5


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


def test_usda_append_and_prepend_rel_order() -> None:
    src = """#usda 1.0
(
    doc = "rel_ops"
)

def "P"
{
    rel ordered = </A>
    append rel ordered = </B>
    prepend rel ordered = </Front>
}
"""
    layer = Layer.new_anonymous("ops")
    assert io.load_from_string(src, layer).ok
    p = Path.from_string("/P")
    tg = layer.get_relationship_targets(p, Token("ordered"))
    assert [t.text() for t in tg] == ["/Front", "/A", "/B"]

    out = io.save_to_string(layer)
    layer2 = Layer.new_anonymous("ops2")
    assert io.load_from_string(out, layer2).ok
    tg2 = layer2.get_relationship_targets(p, Token("ordered"))
    assert [t.text() for t in tg2] == ["/Front", "/A", "/B"]


def test_layer_append_relationship_targets_api() -> None:
    layer = Layer.new_anonymous("api")
    p = Path.from_string("/Q")
    layer.set_relationship_targets(p, Token("r"), [Path.from_string("/1")])
    layer.append_relationship_targets(p, Token("r"), [Path.from_string("/2"), Path.from_string("/3")])
    assert [x.text() for x in layer.get_relationship_targets(p, Token("r"))] == ["/1", "/2", "/3"]


def test_usda_delete_rel_removes_targets() -> None:
    src = """#usda 1.0
(
    doc = "del_rel"
)

def "Prim"
{
    rel mates = [</M1>, </M2>, </M3>]
    delete rel mates = </M2>
}
"""
    layer = Layer.new_anonymous("del")
    assert io.load_from_string(src, layer).ok
    p = Path.from_string("/Prim")
    tg = layer.get_relationship_targets(p, Token("mates"))
    assert [t.text() for t in tg] == ["/M1", "/M3"]


def test_layer_delete_relationship_targets_api() -> None:
    layer = Layer.new_anonymous("del_api")
    p = Path.from_string("/P")
    layer.set_relationship_targets(
        p, Token("k"), [Path.from_string("/a"), Path.from_string("/x"), Path.from_string("/a")]
    )
    layer.delete_relationship_targets(p, Token("k"), [Path.from_string("/a")])
    assert [x.text() for x in layer.get_relationship_targets(p, Token("k"))] == ["/x"]


def test_prim_custom_data_usda_roundtrip() -> None:
    src = """#usda 1.0
(
    doc = "cd"
)

def "R"
(
    customData = {
        string unit = "meter",
        variant = 3,
    }
)
{
}
"""
    layer = Layer.new_anonymous("cd")
    assert io.load_from_string(src, layer).ok
    p = Path.from_string("/R")
    assert layer.has_prim_custom_data_key(p, "unit")
    assert layer.get_prim_custom_data_entry(p, "unit").as_string() == "meter"
    assert layer.get_prim_custom_data_entry(p, "variant").as_double() == 3.0

    out = io.save_to_string(layer)
    layer2 = Layer.new_anonymous("cd2")
    assert io.load_from_string(out, layer2).ok
    assert layer2.get_prim_custom_data_entry(p, "variant").as_double() == 3.0


def test_prim_inherits_specializes_payload_usda_roundtrip() -> None:
    src = """#usda 1.0
(
    doc = "arcs"
)

class "BasePrim"
{
}

def "WithArcs"
(
    inherits = [</BasePrim>]
    prepend specializes = </BasePrim>
    append specializes = </BasePrim>
    payload = @./geom.usda@</Model>
    prepend payload = @./overlay.usda@
)
{
}
"""
    layer = Layer.new_anonymous("arcs")
    assert io.load_from_string(src, layer).ok
    p = Path.from_string("/WithArcs")
    inh = layer.list_prim_inherits(p)
    assert len(inh) == 1 and inh[0].text() == "/BasePrim"
    spe = layer.list_prim_specializes(p)
    assert len(spe) == 2
    assert spe[0].text() == "/BasePrim" and spe[1].text() == "/BasePrim"
    pays = layer.list_payloads(p)
    assert len(pays) == 2
    assert "@./overlay.usda@" in pays[0]
    assert "geom.usda" in pays[1]

    out = io.save_to_string(layer)
    layer2 = Layer.new_anonymous("arcs2")
    assert io.load_from_string(out, layer2).ok
    assert [x.text() for x in layer2.list_prim_inherits(p)] == ["/BasePrim"]


def test_attribute_connection_resolves_on_stage() -> None:
    import freeusd.usd as usd

    src = """#usda 1.0

def "T"
{
    double a = 1.25

    def "Inner"
    {
        double mirror.connect = </T.a>
    }
}
"""
    layer = Layer.new_anonymous("conn")
    assert io.load_from_string(src, layer).ok
    st = usd.Stage.attach_root_layer(layer)
    inner = Path.from_string("/T/Inner")
    prim = st.prim_at(inner)
    assert prim.has_attribute(Token("mirror"))
    v = prim.get_attribute(Token("mirror"))
    assert v is not None
    assert abs(v.as_double() - 1.25) < 1e-12
