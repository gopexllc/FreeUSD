"""UsdLux-shaped schema token stubs."""

from __future__ import annotations

from importlib import import_module

_m = import_module("freeusd._native").usdLux.tokens

DomeLight = _m.DomeLight
SphereLight = _m.SphereLight
DistantLight = _m.DistantLight

__all__ = ["DomeLight", "SphereLight", "DistantLight"]
