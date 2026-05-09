"""UsdRender-shaped schema token stubs."""

from __future__ import annotations

from importlib import import_module

_m = import_module("freeusd._native").usdRender.tokens

Settings = _m.Settings
RenderProduct = _m.RenderProduct

__all__ = ["Settings", "RenderProduct"]
