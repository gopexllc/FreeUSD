from __future__ import annotations

import freeusd


def test_sdf_builtin_tokens_text() -> None:
    assert freeusd.sdf.builtin_tokens.SubLayers().text() == "subLayers"
    assert freeusd.sdf.builtin_tokens.DefaultPrim().text() == "defaultPrim"
    assert freeusd.sdf.builtin_tokens.CustomData().text() == "customData"


def test_native_sdf_builtin_tokens() -> None:
    from freeusd import _native

    assert _native.sdf.builtin_tokens.UpAxis().text() == "upAxis"
