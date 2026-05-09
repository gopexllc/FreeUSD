"""UsdRi-shaped schema token stubs."""

from __future__ import annotations

from importlib import import_module

_m = import_module("freeusd._native").usdRi.tokens

RisObject = _m.RisObject

__all__ = ["RisObject"]
