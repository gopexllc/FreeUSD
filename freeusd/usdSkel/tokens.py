"""UsdSkel-shaped schema token stubs."""

from __future__ import annotations

from importlib import import_module

_m = import_module("freeusd._native").usdSkel.tokens

Skeleton = _m.Skeleton
SkelRoot = _m.SkelRoot
SkelAnimation = _m.SkelAnimation

__all__ = ["Skeleton", "SkelRoot", "SkelAnimation"]
