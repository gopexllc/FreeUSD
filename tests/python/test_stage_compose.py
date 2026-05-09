from __future__ import annotations

import freeusd.pcp as pcp
import freeusd.usd as usd
from freeusd.sdf import Layer, Path
from freeusd.tf import Token
from freeusd.vt import Value


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
