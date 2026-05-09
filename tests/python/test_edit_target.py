from __future__ import annotations

import freeusd


def test_edit_target_default_invalid() -> None:
    et = freeusd.usd.EditTarget()
    assert not et.is_valid()


def test_edit_target_with_layer() -> None:
    layer = freeusd.sdf.Layer.new_anonymous("x")
    et = freeusd.usd.EditTarget(layer)
    assert et.is_valid()
    assert et.layer is layer
