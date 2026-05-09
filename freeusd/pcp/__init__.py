"""Pcp-shaped composition primitives."""

from __future__ import annotations

import importlib

_pcp = importlib.import_module("freeusd._native").pcp

LayerStack = _pcp.LayerStack
compose_sublayers = _pcp.compose_sublayers
compose_sublayers_depth_first = _pcp.compose_sublayers_depth_first

__all__ = ["LayerStack", "compose_sublayers", "compose_sublayers_depth_first"]
