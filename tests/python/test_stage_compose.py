from __future__ import annotations

import os

import freeusd.pcp as pcp
import freeusd.usd as usd
from freeusd.sdf import Layer, Path, PrimSpecifierKind
from freeusd.tf import Token
from freeusd.vt import Value

_FIXTURES = os.path.join(os.path.dirname(__file__), "..", "fixtures")


def test_stage_layer_stack_strongest_attribute_wins() -> None:
    strong = Layer.new_anonymous("s")
    weaker = Layer.new_anonymous("w")
    p = Path.from_string("/A")
    strong.set_field(p, Token("x"), Value.make_double(10.0))
    weaker.set_field(p, Token("x"), Value.make_double(20.0))

    stack = pcp.LayerStack()
    stack.append(strong)
    stack.append(weaker)

    st = usd.Stage.attach_layer_stack(stack)
    assert len(st.compose_layers()) == 2
    v = st.read_field_at_time(p, Token("x"), 1.0)
    assert v is not None
    assert v.as_double() == 10.0


def test_stage_layer_stack_rel_targets_concat_strong_then_weak_order() -> None:
    strong = Layer.new_anonymous("s_rel")
    weaker = Layer.new_anonymous("w_rel")
    p = Path.from_string("/R/prim")
    strong.set_field(p, Token("x"), Value.make_double(1.0))

    stack = pcp.LayerStack()
    stack.append(strong)
    stack.append(weaker)

    strong.set_relationship_targets(
        p, Token("r"), [Path.from_string("/A"), Path.from_string("/B")]
    )
    weaker.set_relationship_targets(p, Token("r"), [Path.from_string("/C")])

    st = usd.Stage.attach_layer_stack(stack)
    prim = st.prim_at(p)
    t = prim.get_relationship_targets(Token("r"))
    assert [tp.text() for tp in t] == ["/A", "/B", "/C"]

    tk = Path.from_string("/Typed")
    weaker.set_prim_kind(tk, Token("Mesh"))
    strong.set_prim_kind(tk, Token("Xform"))
    assert st.resolve_has_prim_kind(tk)
    assert st.resolve_prim_kind(tk).text() == "Xform"
    kprim = st.prim_at(tk)
    assert kprim.has_prim_kind()
    assert kprim.get_prim_kind().text() == "Xform"

    strong.set_prim_active(p, False)
    assert prim.has_prim_active_opinion()
    assert prim.is_active() is False


def test_stage_resolve_prim_defaults_without_opinion() -> None:
    strong = Layer.new_anonymous("s_def")
    stack = pcp.LayerStack()
    stack.append(strong)
    q = Path.from_string("/Solo")

    strong.set_field(q, Token("x"), Value.make_double(0.0))
    st = usd.Stage.attach_layer_stack(stack)
    pq = st.prim_at(q)
    assert pq.is_active() is True
    assert pq.has_prim_active_opinion() is False
    assert st.resolve_prim_active(q) is True
    assert st.resolve_has_prim_active_opinion(q) is False
    assert pq.has_prim_kind() is False
    assert not st.resolve_has_prim_kind(q)


def test_parity_kind_and_active_through_refs_payloads_inherits() -> None:
    path = os.path.normpath(os.path.join(_FIXTURES, "parity_kind_active_refs.usda"))
    st = usd.Stage.open_from_root_file(path)
    assert st is not None
    ref_host = Path.from_string("/World/RefHost")
    payload_host = Path.from_string("/World/PayloadHost")
    inherit_host = Path.from_string("/World/InheritHost")
    assert st.resolve_has_prim_kind(ref_host)
    assert st.resolve_prim_kind(ref_host).text() == "component"
    assert st.prim_at(ref_host).get_prim_kind().text() == "component"
    assert st.resolve_has_prim_active_opinion(ref_host)
    assert st.resolve_prim_active(ref_host) is False
    assert st.prim_at(ref_host).is_active() is False
    assert st.resolve_prim_kind(payload_host).text() == "group"
    assert st.resolve_prim_kind(inherit_host).text() == "assembly"
    assert st.resolve_prim_active(inherit_host) is True
    assert st.resolve_has_prim_active_opinion(inherit_host) is False


def test_parity_custom_data_and_specifier_through_inherits() -> None:
    path = os.path.normpath(os.path.join(_FIXTURES, "parity_custom_data_inherit.usda"))
    st = usd.Stage.open_from_root_file(path)
    assert st is not None
    host = Path.from_string("/World/Host")
    prim = st.prim_at(host)
    assert prim.get_custom_data("role").as_string() == "base"
    assert prim.get_custom_data("priority").as_int32() == 9
    assert prim.has_custom_data_key("role")
    assert set(st.list_composed_prim_custom_data_keys(host)) == {"priority", "role"}
    assert st.resolve_prim_specifier_kind(host) == PrimSpecifierKind.class_
    assert prim.is_abstract()


def test_parity_custom_data_through_references_and_payloads() -> None:
    path = os.path.normpath(os.path.join(_FIXTURES, "parity_custom_data_refs.usda"))
    st = usd.Stage.open_from_root_file(path)
    assert st is not None
    ref_host = Path.from_string("/World/RefHost")
    payload_host = Path.from_string("/World/PayloadHost")
    ref_prim = st.prim_at(ref_host)
    payload_prim = st.prim_at(payload_host)
    assert ref_prim.get_custom_data("role").as_string() == "from_ref"
    assert ref_prim.get_custom_data("priority").as_int32() == 9
    assert payload_prim.get_custom_data("role").as_string() == "from_payload"
    assert payload_prim.get_custom_data("priority").as_int32() == 3
    assert set(st.list_composed_prim_custom_data_keys(ref_host)) == {"priority", "role"}
    assert set(st.list_composed_prim_custom_data_keys(payload_host)) == {"priority", "role"}


def test_parity_variant_selection_through_payload() -> None:
    path = os.path.normpath(os.path.join(_FIXTURES, "parity_variant_selection_payloads.usda"))
    st = usd.Stage.open_from_root_file(path)
    assert st is not None
    host = Path.from_string("/World/PayloadHost")
    assert st.get_composed_prim_variant_selection(host, "modelVariant") == "B"
    assert set(st.list_composed_prim_variant_selection_sets(host)) == {"modelVariant"}
    value = st.read_field_at_time(host, Token("variantValue"), 1.0)
    assert value is not None
    assert value.as_double() == 9.0


def test_inherit_instance_prim_authored_specifier_not_class() -> None:
    path = os.path.normpath(os.path.join(_FIXTURES, "parity_physics_collision_inherit.usda"))
    st = usd.Stage.open_from_root_file(path)
    assert st is not None
    collider = st.prim_at(Path.from_string("/World/Collider"))
    collision_class = st.prim_at(Path.from_string("/World/CollisionClass"))
    assert collision_class.is_abstract()
    assert not collision_class.is_instance_prim()
    assert collider.is_instance_prim()
