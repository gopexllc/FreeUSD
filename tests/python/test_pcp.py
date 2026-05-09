from __future__ import annotations

import freeusd.pcp as pcp
from freeusd.sdf import Layer, Path
from freeusd.tf import Token


def test_compose_sublayers_orders_stack() -> None:
    root = Layer.new_anonymous("root.usda")
    sub_a = Layer.new_anonymous("./a.usda")
    sub_b = Layer.new_anonymous("./b.usda")

    root.set_sub_layers(["./a.usda", "./none.usda", "./b.usda"])

    catalog = {"./a.usda": sub_a, "./b.usda": sub_b}

    def resolve(authored_path: str) -> Layer | None:
        return catalog.get(authored_path)

    stack = pcp.compose_sublayers(root, resolve)
    assert not stack.is_empty()
    lys = stack.layers()
    assert len(lys) == 3
    assert lys[0] is root
    assert lys[1] is sub_a
    assert lys[2] is sub_b

    stack.clear()
    assert stack.is_empty()


def test_layer_binding_set_clear_references() -> None:
    layer = Layer.new_anonymous("x")
    p = Path.from_string("/R")
    layer.set_prim_kind(p, Token("Mesh"))
    layer.set_references(p, ["@scene.usda@</World>", "@other.usda@"])
    refs = layer.list_references(p)
    assert "</World>" in refs[0] and "scene.usda" in refs[0]
    assert len(refs) == 2
    plist = layer.list_prim_references(p)
    assert plist[0].asset_path == "scene.usda"
    assert plist[0].prim_path is not None and plist[0].prim_path.text() == "/World"
    layer.clear_references(p)
    assert layer.list_references(p) == []


def test_compose_sublayers_depth_first_order() -> None:
    root = Layer.new_anonymous("r")
    sx = Layer.new_anonymous("x")
    sy = Layer.new_anonymous("y")
    root.set_sub_layers(["x.usda"])
    sx.set_sub_layers(["y.usda"])

    def resolve(authored_path: str) -> Layer | None:
        return {"x.usda": sx, "y.usda": sy}.get(authored_path)

    stack = pcp.compose_sublayers_depth_first(root, resolve)
    ly = stack.layers()
    assert len(ly) == 3 and ly[0] is root and ly[1] is sx and ly[2] is sy


def test_layer_primitive_flags() -> None:
    layer = Layer.new_anonymous("x")
    assert not layer.has_default_prim()
    layer.set_default_prim("World")
    assert layer.has_default_prim()
    layer.clear_default_prim()
    assert not layer.has_default_prim()

    p = Path.from_string("/P")
    layer.set_prim_active(p, False)
    assert layer.has_prim_active_opinion(p)
    assert layer.is_prim_active(p) is False
    layer.set_prim_active(p, True)
    assert not layer.has_prim_active_opinion(p)
    assert layer.is_prim_active(p)
