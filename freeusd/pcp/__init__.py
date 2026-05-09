"""Pcp-shaped composition primitives."""

from __future__ import annotations

import importlib

_pcp = importlib.import_module("freeusd._native").pcp

LayerStack = _pcp.LayerStack

__all__ = ["LayerStack"]
