def test_layer_pseudoroot_metadata_usda_roundtrip():
    import freeusd.io as io
    from freeusd.sdf import Layer, Path

    src = """#usda 1.0
(
    startTimeCode = 0
    endTimeCode = 100
    timeCodesPerSecond = 30
    framesPerSecond = 30
    framePrecision = 2
    metersPerUnit = 0.01
    upAxis = "Z"
    primOrder = [</Root/A>, </Root>]
)

def Xform "Root"
{
    def "A" {}
}
"""
    layer = Layer.new_anonymous("pseudo")
    assert io.load_from_string(src, layer).ok
    assert layer.start_time_code() == 0.0
    assert layer.end_time_code() == 100.0
    assert layer.time_codes_per_second() == 30.0
    assert layer.frames_per_second() == 30.0
    assert layer.frame_precision() == 2
    assert layer.meters_per_unit() == 0.01
    assert layer.up_axis() == "Z"
    po = layer.prim_order()
    assert len(po) == 2
    assert po[0] == Path.from_string("/Root/A")
    assert po[1] == Path.from_string("/Root")

    out = io.save_to_string(layer)
    layer2 = Layer.new_anonymous("pseudo2")
    assert io.load_from_string(out, layer2).ok
    assert layer2.start_time_code() == 0.0
    assert layer2.end_time_code() == 100.0
    assert layer2.time_codes_per_second() == 30.0
    assert layer2.frames_per_second() == 30.0
    assert layer2.frame_precision() == 2
    assert layer2.meters_per_unit() == 0.01
    assert layer2.up_axis() == "Z"
    po2 = layer2.prim_order()
    assert len(po2) == 2
    assert po2[0] == Path.from_string("/Root/A")
    assert po2[1] == Path.from_string("/Root")


def test_set_pseudoroot_apis():
    from freeusd.sdf import Layer, Path

    layer = Layer.new_anonymous("api")
    layer.set_start_time_code(1.0)
    layer.set_end_time_code(99.0)
    layer.set_time_codes_per_second(24.0)
    layer.set_frames_per_second(24.0)
    layer.set_frame_precision(4)
    layer.set_meters_per_unit(1.0)
    layer.set_up_axis("Y")
    layer.set_prim_order([Path.from_string("/A"), Path.from_string("/B")])
    assert layer.start_time_code() == 1.0
    assert layer.end_time_code() == 99.0
    assert layer.time_codes_per_second() == 24.0
    assert layer.frames_per_second() == 24.0
    assert layer.frame_precision() == 4
    assert layer.meters_per_unit() == 1.0
    assert layer.up_axis() == "Y"
    assert [p.text() for p in layer.prim_order()] == ["/A", "/B"]
    layer.clear_start_time_code()
    assert layer.start_time_code() is None
    layer.clear_prim_order()
    assert layer.prim_order() == []
