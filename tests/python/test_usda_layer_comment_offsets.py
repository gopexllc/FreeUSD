def test_layer_comment_and_sub_layer_offsets_usda():
    import freeusd.io as io
    from freeusd.sdf import Layer, LayerOffset

    src = """#usda 1.0
(
    comment = "note"
    subLayers = [@./ref.usda@]
    subLayerOffsets = {
        @./ref.usda@: (10, 1),
    }
)

def "Root" {}
"""
    layer = Layer.new_anonymous("c")
    assert io.load_from_string(src, layer).ok
    assert layer.comment() == "note"
    assert layer.sub_layers() == ["./ref.usda"]
    off = layer.get_sub_layer_offset("./ref.usda")
    assert isinstance(off, LayerOffset)
    assert off.offset == 10 and off.scale == 1

    out = io.save_to_string(layer)
    layer2 = Layer.new_anonymous("c2")
    assert io.load_from_string(out, layer2).ok
    assert layer2.comment() == "note"
    assert layer2.get_sub_layer_offset("./ref.usda").offset == 10


def test_set_sub_layer_offset_api():
    from freeusd.sdf import Layer, LayerOffset

    layer = Layer.new_anonymous("x")
    layer.set_sub_layer_offset("./a.usda", LayerOffset(2, 0.5))
    pairs = layer.list_sub_layer_offsets()
    assert len(pairs) == 1
    assert pairs[0][0] == "./a.usda"
    assert pairs[0][1].offset == 2 and pairs[0][1].scale == 0.5
    layer.erase_sub_layer_offset("./a.usda")
    assert layer.list_sub_layer_offsets() == []
