"""UsdShade-shaped schema helpers (tokens plus minimal material/shader runtime)."""

from __future__ import annotations

from importlib import import_module

from . import tokens

_shade = import_module("freeusd._native").usdShade

Material = _shade.Material
Shader = _shade.Shader
PreviewSurface = _shade.PreviewSurface

__all__ = [
    "Material",
    "PreviewSurface",
    "Shader",
    "tokens",
]
