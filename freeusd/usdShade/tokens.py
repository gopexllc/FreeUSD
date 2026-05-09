"""UsdShade-shaped schema token stubs."""

from __future__ import annotations

from importlib import import_module

_m = import_module("freeusd._native").usdShade.tokens

Material = _m.Material
Shader = _m.Shader
NodeGraph = _m.NodeGraph

__all__ = ["Material", "Shader", "NodeGraph"]
