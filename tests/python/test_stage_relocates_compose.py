def test_composed_relocates_strongest_wins():
    import freeusd.pcp as pcp
    import freeusd.usd as usd
    from freeusd.sdf import Layer, Path

    strong = Layer.new_anonymous("s")
    weak = Layer.new_anonymous("w")
    src = Path.from_string("/Src")
    strong.set_relocate(src, Path.from_string("/StrongDst"))
    weak.set_relocate(src, Path.from_string("/WeakDst"))
    weak.set_relocate(Path.from_string("/WeakOnly"), Path.from_string("/W"))

    stack = pcp.LayerStack()
    stack.append(strong)
    stack.append(weak)
    stage = usd.Stage.attach_layer_stack(stack)

    assert stage.get_composed_relocate_target(src) == Path.from_string("/StrongDst")
    assert stage.get_composed_relocate_target(Path.from_string("/WeakOnly")) == Path.from_string("/W")
    assert stage.relocate_source_in_any_layer(src)
    pairs = stage.list_composed_relocates()
    assert len(pairs) == 2


def test_composed_prefix_substitutions_strongest_wins():
    import freeusd.pcp as pcp
    import freeusd.usd as usd
    from freeusd.sdf import Layer

    strong = Layer.new_anonymous("s")
    weak = Layer.new_anonymous("w")
    strong.set_prefix_substitution("/Models", "/ModelsV2")
    weak.set_prefix_substitution("/Models", "/ModelsWeak")
    weak.set_prefix_substitution("/OnlyW", "/W")

    stack = pcp.LayerStack()
    stack.append(strong)
    stack.append(weak)
    stage = usd.Stage.attach_layer_stack(stack)

    assert stage.get_composed_prefix_substitution("/Models") == "/ModelsV2"
    assert stage.get_composed_prefix_substitution("/OnlyW") == "/W"
    assert stage.prefix_substitution_key_in_any_layer("/Models")
    pairs = stage.list_composed_prefix_substitutions()
    assert len(pairs) == 2
