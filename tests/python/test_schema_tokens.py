"""Schema stub tokens: Python layout mirrors OpenUSD-style `*.tokens` modules."""

from __future__ import annotations

import freeusd


def test_usd_geom_tokens_text() -> None:
    assert freeusd.usdGeom.tokens.Mesh().text() == "Mesh"
    assert freeusd.usdGeom.token_mesh().text() == "Mesh"


def test_usd_shade_tokens_text() -> None:
    assert freeusd.usdShade.tokens.Material().text() == "Material"


def test_usd_physics_scene_token_text() -> None:
    assert freeusd.usdPhysics.tokens.Scene().text() == "PhysicsScene"


def test_usd_render_settings_token_text() -> None:
    assert freeusd.usdRender.tokens.Settings().text() == "RenderSettings"


def test_native_nested_tokens() -> None:
    from freeusd import _native

    assert _native.usdVol.tokens.OpenVDBAsset().text() == "OpenVDBAsset"
    assert _native.usdRi.tokens.RisObject().text() == "RisObject"


def test_usd_kind_tokens_text() -> None:
    assert freeusd.usd.kind_tokens.Component().text() == "component"
    assert freeusd.usd.kind_tokens.Assembly().text() == "assembly"
