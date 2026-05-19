"""UsdVol-shaped schema helpers (tokens plus minimal OpenVDBAsset runtime)."""

from __future__ import annotations

from importlib import import_module

from . import tokens

_vol = import_module("freeusd._native").usdVol

OpenVDBAsset = _vol.OpenVDBAsset

__all__ = [
    "OpenVDBAsset",
    "tokens",
]
