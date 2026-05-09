def test_custom_layer_data_usda_roundtrip():
    import freeusd.io as io
    from freeusd.sdf import Layer

    src = """#usda 1.0
(
    customLayerData = {
        int layerBuild = 7,
        string layerTag = "x",
    }
)

def "R" {}
"""
    layer = Layer.new_anonymous("cld")
    assert io.load_from_string(src, layer).ok
    assert layer.has_custom_layer_data_key("layerBuild")
    assert layer.has_custom_layer_data_key("layerTag")
    assert layer.list_custom_layer_data_keys() == ["layerBuild", "layerTag"]

    out = io.save_to_string(layer)
    layer2 = Layer.new_anonymous("cld2")
    assert io.load_from_string(out, layer2).ok
    assert layer2.get_custom_layer_data_entry("layerTag").as_string() == "x"


def test_composed_custom_layer_data():
    import freeusd.pcp as pcp
    import freeusd.usd as usd
    from freeusd.sdf import Layer
    from freeusd.vt import Value

    strong = Layer.new_anonymous("s")
    weak = Layer.new_anonymous("w")
    strong.set_custom_layer_data_entry("k", Value.make_string("strong"))
    weak.set_custom_layer_data_entry("k", Value.make_string("weak"))
    weak.set_custom_layer_data_entry("onlyWeak", Value.make_int32(3))

    stack = pcp.LayerStack()
    stack.append(strong)
    stack.append(weak)
    stage = usd.Stage.attach_layer_stack(stack)

    assert stage.get_composed_custom_layer_data("k").as_string() == "strong"
    assert stage.custom_layer_data_key_in_any_layer("onlyWeak")
    keys = stage.list_composed_custom_layer_data_keys()
    assert "k" in keys and "onlyWeak" in keys
