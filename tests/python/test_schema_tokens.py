"""Schema stub tokens: Python layout mirrors OpenUSD-style `*.tokens` modules."""

from __future__ import annotations

import freeusd


def test_usd_geom_tokens_text() -> None:
    assert freeusd.usdGeom.tokens.Mesh().text() == "Mesh"
    assert freeusd.usdGeom.token_mesh().text() == "Mesh"


def test_usd_shade_tokens_text() -> None:
    assert freeusd.usdShade.tokens.Material().text() == "Material"


def test_usd_physics_scene_token_text() -> None:
    assert freeusd.usdPhysics.tokens.PhysicsScene().text() == "PhysicsScene"


def test_usd_render_settings_token_text() -> None:
    assert freeusd.usdRender.tokens.RenderSettings().text() == "RenderSettings"


def test_native_nested_tokens() -> None:
    from freeusd import _native

    assert _native.usdVol.tokens.OpenVDBAsset().text() == "OpenVDBAsset"
    assert _native.usdRi.tokens.RiMaterialAPI().text() == "RiMaterialAPI"
    assert getattr(_native.usdUI.tokens, "ui:displayName")().text() == "ui:displayName"
    assert _native.usdHydra.tokens.HydraGenerativeProceduralAPI().text() == "HydraGenerativeProceduralAPI"


def test_usd_kind_tokens_text() -> None:
    assert freeusd.usd.kind_tokens.Component().text() == "component"
    assert freeusd.usd.kind_tokens.Assembly().text() == "assembly"


def test_usd_schema_data_tokens_from_usd_lib() -> None:
    assert freeusd.usd.schema_data_tokens.ModelAPI().text() == "ModelAPI"
    assert freeusd.usd.schema_data_tokens.CollectionAPI().text() == "CollectionAPI"


def test_schema_token_subpackages_exported() -> None:
    """Top-level packages mirror pxr schema libs that ship generatedSchema.usda."""
    for name in (
        "usdHydra",
        "usdMtlx",
        "usdProc",
        "usdSemantics",
        "usdUI",
    ):
        mod = getattr(freeusd, name)
        assert hasattr(mod, "tokens"), name
        public = [n for n in dir(mod.tokens) if not n.startswith("_")]
        callables = [n for n in public if callable(getattr(mod.tokens, n))]
        assert callables, f"{name}.tokens has no public callables"
