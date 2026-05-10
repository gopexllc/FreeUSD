from __future__ import annotations

import freeusd
from freeusd.sdf import Layer, Path, PrimReference, PrimSpecifierKind
from freeusd.tf import Token
from freeusd.usd import Stage
from freeusd.vt import Value


def test_stage_prim_roundtrip() -> None:
    layer = Layer.new_anonymous("test")
    cube = Path.from_string("/World/Cube")
    layer.set_field(cube, Token("size"), Value.make_double(3.0))
    layer.set_default_prim("World")

    stage = Stage.attach_root_layer(layer)
    assert stage is not None

    prim = stage.prim_at(cube)
    assert prim.is_valid()
    assert prim.name() == "Cube"
    assert prim.parent().is_valid()
    assert prim.parent().path().text() == "/World"
    assert prim.stage() is stage
    dp = stage.default_prim()
    assert dp is not None
    assert dp.is_valid()
    assert dp.path().text() == "/World"
    assert dp.name() == "World"
    assert len(stage.prim_at(Path.from_string("/World")).children()) >= 1
    assert prim.has_attribute(Token("size"))
    v = prim.get_attribute(Token("size"))
    assert not v.is_empty()
    assert v.as_double() == 3.0


def test_prim_get_attribute_at_time() -> None:
    layer = Layer.new_anonymous("time")
    cube = Path.from_string("/World/Cube")
    layer.set_field(cube, Token("height"), Value.make_double(0.0))
    layer.set_time_sample(cube, Token("height"), 1.0, Value.make_double(10.0))
    layer.set_time_sample(cube, Token("height"), 3.0, Value.make_double(30.0))
    stage = Stage.attach_root_layer(layer)
    prim = stage.prim_at(cube)
    assert prim.get_attribute(Token("height"), 2.0).as_double() == 10.0
    assert prim.get_attribute(Token("height"), 3.0).as_double() == 30.0


def test_prim_references_inherits_payloads_compose() -> None:
    from freeusd.pcp import LayerStack

    strong = Layer.new_anonymous("arc_s")
    weak = Layer.new_anonymous("arc_w")
    px = Path.from_string("/Root/X")
    strong.set_field(px, Token("strength"), Value.make_double(1.0))
    weak.set_field(px, Token("strength"), Value.make_double(99.0))
    strong.add_prim_reference(px, PrimReference("./a.usda"))
    weak.add_prim_reference(px, PrimReference("./b.usda"))
    strong.add_prim_inherit(px, Path.from_string("/BaseA"))
    weak.add_prim_inherit(px, Path.from_string("/BaseB"))
    strong.add_prim_specializes(px, Path.from_string("/SpecA"))
    weak.add_prim_specializes(px, Path.from_string("/SpecB"))
    strong.add_prim_payload(px, PrimReference("./p.usdc"))

    stack = LayerStack()
    stack.append(strong)
    stack.append(weak)
    st = Stage.attach_layer_stack(stack)
    assert len(st.read_prim_references(px)) == 2
    assert st.read_prim_references(px)[0].asset_path == "./a.usda"
    assert st.read_prim_references(px)[1].asset_path == "./b.usda"
    prim = st.prim_at(px)
    assert prim.has_references() and len(prim.get_references()) == 2
    assert len(st.read_prim_inherits(px)) == 2
    assert prim.has_inherits()
    assert len(st.read_prim_specializes(px)) == 2
    assert st.read_prim_specializes(px)[0] == Path.from_string("/SpecA")
    assert prim.has_specializes() and len(prim.get_specializes()) == 2
    assert len(st.read_prim_payloads(px)) == 1
    assert st.read_prim_payloads(px)[0].asset_path == "./p.usdc"
    assert prim.has_payloads()


def test_prim_list_names_connection_sample_times() -> None:
    pipe = Path.from_string("/World/Pipe")
    sink = Path.from_string("/World/Sink")
    layer = Layer.new_anonymous("conn")
    layer.set_field(pipe, Token("sourceValue"), Value.make_double(2.0))
    layer.set_field(sink, Token("ported"), Value.make_double(0.0))
    layer.set_attribute_connection(sink, Token("ported"), Path.from_string("/World/Pipe.sourceValue"))
    layer.set_time_sample(sink, Token("ported"), 0.0, Value.make_double(1.0))
    layer.set_time_sample(sink, Token("ported"), 2.0, Value.make_double(2.0))

    stage = Stage.attach_root_layer(layer)
    sp = stage.prim_at(sink)
    names = sp.list_attribute_names()
    assert "ported" in names
    assert sp.has_attribute_connection(Token("ported"))
    tgt = sp.get_attribute_connection_target(Token("ported"))
    assert tgt is not None
    assert tgt.text() == "/World/Pipe.sourceValue"
    times = sp.list_attribute_sample_times(Token("ported"))
    assert 0.0 in times and 2.0 in times


def test_resolve_prim_specifier_and_is_abstract() -> None:
    from freeusd.pcp import LayerStack

    strong = Layer.new_anonymous("spec_s")
    weak = Layer.new_anonymous("spec_w")
    px = Path.from_string("/Root/X")
    strong.set_field(px, Token("strength"), Value.make_double(1.0))
    weak.set_field(px, Token("strength"), Value.make_double(99.0))
    weak.set_prim_specifier(px, PrimSpecifierKind.over)

    stack = LayerStack()
    stack.append(strong)
    stack.append(weak)
    stage = Stage.attach_layer_stack(stack)
    assert stage.resolve_prim_specifier_kind(px) == PrimSpecifierKind.over
    assert not stage.prim_at(px).is_abstract()

    q = Path.from_string("/ClsPrim")
    weak.set_prim_specifier(q, PrimSpecifierKind.class_)
    weak.set_field(q, Token("n"), Value.make_int32(1))
    strong.set_field(q, Token("n"), Value.make_int32(0))
    assert stage.resolve_prim_specifier_kind(q) == PrimSpecifierKind.class_
    assert stage.prim_at(q).is_abstract()


def test_traverse_preorder() -> None:
    layer = Layer.new_anonymous("tr")
    layer.set_field(Path.from_string("/World/Cube"), Token("size"), Value.make_double(1.0))
    stage = Stage.attach_root_layer(layer)
    paths: list[str] = []

    def vis(p):
        paths.append(p.path().text())
        return True

    stage.traverse_preorder(vis)
    assert "/World" in paths
    assert "/World/Cube" in paths

    paths2: list[str] = []

    def prune_world(p):
        paths2.append(p.path().text())
        return p.path().text() != "/World"

    stage.traverse_preorder(prune_world)
    assert "/World" in paths2
    assert "/World/Cube" not in paths2


def test_path_parse() -> None:
    p = Path.from_string("/World/Cube.xformOp:translate")
    assert p.is_property_path()
    assert p.prim_path().text() == "/World/Cube"


def test_usd_geom_xformable_local_to_world() -> None:
    from freeusd.gf import Matrix4d
    from freeusd.usdGeom import Xformable

    layer = Layer.new_anonymous("xf")
    world = Path.from_string("/World")
    cube = Path.from_string("/World/Cube")
    layer.set_field(world, Token("xformOp:translate"), Value.make_vec3d(10.0, 0.0, 0.0))
    layer.set_field(cube, Token("xformOp:translate"), Value.make_vec3d(1.0, 2.0, 3.0))
    layer.set_default_prim("World")
    stage = Stage.attach_root_layer(layer)
    prim = stage.prim_at(cube)
    xf = Xformable(prim)
    mw = xf.compute_local_to_world_transform(1.0)
    expect = Matrix4d.Multiply(Matrix4d.Translate(10.0, 0.0, 0.0), Matrix4d.Translate(1.0, 2.0, 3.0))
    assert mw.as_list() == expect.as_list()


def test_version_export() -> None:
    assert freeusd.version()
    assert freeusd.version_tuple()[0] >= 0


def test_stage_root_pseudoroot_from_strongest_layer() -> None:
    from freeusd.pcp import LayerStack

    strong = Layer.new_anonymous("s")
    weak = Layer.new_anonymous("w")
    strong.set_meters_per_unit(0.01)
    strong.set_up_axis("Y")
    strong.set_start_time_code(0.0)
    weak.set_meters_per_unit(1.0)
    weak.set_up_axis("Z")

    stack = LayerStack()
    stack.append(strong)
    stack.append(weak)
    stage = Stage.attach_layer_stack(stack)
    assert stage is not None
    assert stage.meters_per_unit() == 0.01
    assert stage.up_axis() == "Y"
    assert stage.start_time_code() == 0.0
    assert stage.end_time_code() is None
    assert stage.prim_order() == []


def test_stage_pseudoroot_fallback_weaker_layer() -> None:
    from freeusd.pcp import LayerStack
    from freeusd.sdf import Path

    strong = Layer.new_anonymous("s")
    weak = Layer.new_anonymous("w")
    weak.set_meters_per_unit(0.1)
    weak.set_up_axis("Z")
    weak.set_start_time_code(2.0)
    weak.set_prim_order([Path.from_string("/W")])

    stack = LayerStack()
    stack.append(strong)
    stack.append(weak)
    stage = Stage.attach_layer_stack(stack)
    assert stage.meters_per_unit() == 0.1
    assert stage.up_axis() == "Z"
    assert stage.start_time_code() == 2.0
    assert len(stage.prim_order()) == 1
    assert stage.prim_order()[0].text() == "/W"
