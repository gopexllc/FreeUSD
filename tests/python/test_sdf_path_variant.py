def test_path_variant_selections():
    from freeusd.sdf import Path

    p = Path.from_string("/World/Mesh{lod=high}")
    assert p.name() == "Mesh"
    assert p.variant_selections() == [("lod", "high")]

    p2 = Path.from_string("/A{a=1}/B{b=2}")
    assert p2.variant_selections() == [("a", "1"), ("b", "2")]

    assert Path.from_string("/Plain").variant_selections() == []
