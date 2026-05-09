"""Vt-shaped typed values."""

from __future__ import annotations

import importlib

_vt = importlib.import_module("freeusd._native").vt

Value = _vt.Value

__all__ = ["Value"]
