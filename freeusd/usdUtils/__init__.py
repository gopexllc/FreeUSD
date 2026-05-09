"""UsdUtils-shaped helpers (clean-room; grows with flatten/cache APIs)."""

from __future__ import annotations

import importlib

_utils = importlib.import_module("freeusd._native").usdUtils

FlattenOptions = _utils.FlattenOptions

__all__ = ["FlattenOptions"]
