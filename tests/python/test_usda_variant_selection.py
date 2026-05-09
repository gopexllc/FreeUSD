def test_usda_variant_selection_roundtrip():
    import freeusd.io as io
    from freeusd.sdf import Layer, Path

    text = """#usda 1.0
(
)
def Xform "Root"
(
    variantSelection = {
        string modelVariant = "HiPoly",
    }
)
{
}
"""
    layer = Layer.new_anonymous("t")
    r = io.load_from_string(text, layer)
    assert r.ok
    p = Path.from_string("/Root")
    assert layer.has_prim_variant_selection_key(p, "modelVariant")
    assert layer.get_prim_variant_selection_entry(p, "modelVariant") == "HiPoly"

    out = io.save_to_string(layer)
    layer2 = Layer.new_anonymous("t2")
    r2 = io.load_from_string(out, layer2)
    assert r2.ok
    assert layer2.get_prim_variant_selection_entry(p, "modelVariant") == "HiPoly"


def test_layer_variant_selection_api():
    from freeusd.sdf import Layer, Path

    layer = Layer.new_anonymous("x")
    p = Path.from_string("/Prim")
    layer.set_prim_variant_selection_entry(p, "vset", "choice")
    assert layer.list_prim_variant_selection_sets(p) == ["vset"]
    layer.erase_prim_variant_selection_entry(p, "vset")
    assert layer.list_prim_variant_selection_sets(p) == []
