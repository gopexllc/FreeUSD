"""UsdLux-shaped schema helpers (tokens plus minimal DistantLight runtime)."""

from __future__ import annotations

from importlib import import_module

from . import tokens

_lux = import_module("freeusd._native").usdLux

DistantLight = _lux.DistantLight
SphereLight = _lux.SphereLight
RectLight = _lux.RectLight
DiskLight = _lux.DiskLight
CylinderLight = _lux.CylinderLight
DomeLight = _lux.DomeLight

__all__ = [
    "CylinderLight",
    "DistantLight",
    "DiskLight",
    "DomeLight",
    "RectLight",
    "SphereLight",
    "tokens",
]
