from __future__ import annotations

import freeusd


def test_usd_builtin_tokens_text() -> None:
    assert freeusd.usd.builtin_tokens.References().text() == "references"
    assert freeusd.usd.builtin_tokens.Payload().text() == "payload"
    assert freeusd.usd.builtin_tokens.VariantSets().text() == "variantSets"


def test_native_usd_builtin_tokens() -> None:
    from freeusd import _native

    assert _native.usd.builtin_tokens.Inherits().text() == "inherits"
