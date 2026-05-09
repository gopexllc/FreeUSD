def test_variant_sets_roundtrip():
    import freeusd.io as io
    from freeusd.sdf import Layer, Path

    src = """#usda 1.0
(
)
def "Root"
(
    variantSets = {
        modelVariant = {
            "A" = {},
            B = {},
        }
    }
)
{
}
"""
    layer = Layer.new_anonymous("vs")
    assert io.load_from_string(src, layer).ok
    p = Path.from_string("/Root")
    assert layer.has_prim_variant_set(p, "modelVariant")
    assert layer.list_prim_variant_names(p, "modelVariant") == ["A", "B"]

    out = io.save_to_string(layer)
    layer2 = Layer.new_anonymous("vs2")
    assert io.load_from_string(out, layer2).ok
    assert layer2.list_prim_variant_names(p, "modelVariant") == ["A", "B"]


def test_set_prim_variant_set_variants_api():
    from freeusd.sdf import Layer, Path

    layer = Layer.new_anonymous("x")
    p = Path.from_string("/P")
    layer.set_prim_variant_set_variants(p, "vset", ["x", "y"])
    assert layer.list_prim_variant_set_names(p) == ["vset"]
    layer.set_prim_variant_set_variants(p, "vset", [])
    assert layer.list_prim_variant_set_names(p) == []
