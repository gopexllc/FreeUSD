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


def test_layer_prefix_substitutions_usda():
    import freeusd.io as io
    from freeusd.sdf import Layer

    src = """#usda 1.0
(
    prefixSubstitutions = {
        string "/Old": "/New",
        "/Geom": "/Assets",
    }
)

def "Root" {}
"""
    layer = Layer.new_anonymous("p")
    assert io.load_from_string(src, layer).ok
    assert layer.has_prefix_substitution("/Old")
    assert layer.get_prefix_substitution("/Old") == "/New"
    assert layer.get_prefix_substitution("/Geom") == "/Assets"
    assert layer.list_prefix_substitutions() == [("/Geom", "/Assets"), ("/Old", "/New")]

    out = io.save_to_string(layer)
    layer2 = Layer.new_anonymous("p2")
    assert io.load_from_string(out, layer2).ok
    assert layer2.get_prefix_substitution("/Old") == "/New"


def test_set_prefix_substitution_api():
    from freeusd.sdf import Layer

    layer = Layer.new_anonymous("x")
    layer.set_prefix_substitution("/A", "/B")
    assert layer.list_prefix_substitutions() == [("/A", "/B")]
    layer.erase_prefix_substitution("/A")
    assert layer.list_prefix_substitutions() == []
