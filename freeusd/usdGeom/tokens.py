"""UsdGeom-shaped schema token stubs (mirrors C++ `freeusd::usdGeom::tokens`)."""

from __future__ import annotations

from importlib import import_module

_m = import_module("freeusd._native").usdGeom.tokens

Mesh = _m.Mesh
Xform = _m.Xform
Scope = _m.Scope

__all__ = ["Mesh", "Xform", "Scope"]
