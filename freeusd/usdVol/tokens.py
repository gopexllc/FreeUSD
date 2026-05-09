"""UsdVol-shaped schema token stubs."""

from __future__ import annotations

from importlib import import_module

_m = import_module("freeusd._native").usdVol.tokens

OpenVDBAsset = _m.OpenVDBAsset
Field3DAsset = _m.Field3DAsset

__all__ = ["OpenVDBAsset", "Field3DAsset"]
