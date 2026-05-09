from __future__ import annotations

import freeusd


def test_asset_path_python() -> None:
    a = freeusd.sdf.AssetPath()
    assert a.is_empty()

    b = freeusd.sdf.AssetPath("./x.usda")
    assert b.path() == "./x.usda"
    b.set_path("y.usdc")
    assert b.path() == "y.usdc"
    assert b == freeusd.sdf.AssetPath("y.usdc")
