"""UsdMedia-shaped schema token stubs."""

from __future__ import annotations

from importlib import import_module

_m = import_module("freeusd._native").usdMedia.tokens

SpatialAudio = _m.SpatialAudio

__all__ = ["SpatialAudio"]
