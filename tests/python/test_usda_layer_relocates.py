def test_layer_relocates_usda():
    import freeusd.io as io
    from freeusd.sdf import Layer, Path

    src = """#usda 1.0
(
    relocates = {
        </Root/A>: </Root/B>,
    }
)

def "Root" {
    def "A" {}
    def "B" {}
}
"""
    layer = Layer.new_anonymous("r")
    assert io.load_from_string(src, layer).ok
    a = Path.from_string("/Root/A")
    b = Path.from_string("/Root/B")
    assert layer.has_relocate(a)
    assert layer.get_relocate_target(a) == b
    assert layer.list_relocates() == [(a, b)]

    out = io.save_to_string(layer)
    layer2 = Layer.new_anonymous("r2")
    assert io.load_from_string(out, layer2).ok
    assert layer2.get_relocate_target(a) == b


def test_set_relocate_api():
    from freeusd.sdf import Layer, Path

    layer = Layer.new_anonymous("x")
    f = Path.from_string("/From")
    t = Path.from_string("/To")
    layer.set_relocate(f, t)
    assert layer.list_relocates() == [(f, t)]
    layer.erase_relocate(f)
    assert layer.list_relocates() == []
