"""UsdLux-shaped schema helpers (tokens plus minimal DistantLight runtime)."""

from __future__ import annotations

from importlib import import_module

from . import tokens

_lux = import_module("freeusd._native").usdLux

DistantLight = _lux.DistantLight

__all__ = [
    "DistantLight",
    "tokens",
]
