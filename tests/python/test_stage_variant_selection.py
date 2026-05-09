def test_composed_variant_selection_strongest_wins():
    import freeusd.pcp as pcp
    import freeusd.usd as usd
    from freeusd.sdf import Layer, Path

    strong = Layer.new_anonymous("s")
    weak = Layer.new_anonymous("w")
    p = Path.from_string("/Root/P")
    strong.set_prim_variant_selection_entry(p, "model", "A")
    weak.set_prim_variant_selection_entry(p, "model", "B")
    weak.set_prim_variant_selection_entry(p, "only_weak", "Z")

    stack = pcp.LayerStack()
    stack.append(strong)
    stack.append(weak)
    stage = usd.Stage.attach_layer_stack(stack)

    assert stage.get_composed_prim_variant_selection(p, "model") == "A"
    assert stage.get_composed_prim_variant_selection(p, "only_weak") == "Z"
    assert set(stage.list_composed_prim_variant_selection_sets(p)) == {"model", "only_weak"}

    prim = stage.prim_at(p)
    assert prim.get_variant_selection("model") == "A"
    assert prim.has_variant_selection_key("only_weak")
    assert set(prim.list_variant_selection_sets()) == {"model", "only_weak"}

    strong.set_prim_variant_set_variants(p, "geo", ["A", "B"])
    weak.set_prim_variant_set_variants(p, "geo", ["Z"])
    weak.set_prim_variant_set_variants(p, "other", ["x"])
    assert set(stage.list_composed_prim_variant_set_names(p)) == {"geo", "other"}
    assert stage.get_composed_prim_variant_names(p, "geo") == ["A", "B"]
    assert stage.get_composed_prim_variant_names(p, "other") == ["x"]
    assert prim.has_variant_set("geo")
    assert prim.list_variant_names("geo") == ["A", "B"]
