"""Usd-shaped stage API."""

from __future__ import annotations

import importlib

_usd = importlib.import_module("freeusd._native").usd

Prim = _usd.Prim
Stage = _usd.Stage

__all__ = ["Prim", "Stage"]
