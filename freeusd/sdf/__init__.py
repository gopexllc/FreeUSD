"""Sdf-shaped scene description (paths, layers)."""

from __future__ import annotations

import importlib

_sdf = importlib.import_module("freeusd._native").sdf

Layer = _sdf.Layer
Path = _sdf.Path
PrimSpecifierKind = _sdf.PrimSpecifierKind

__all__ = ["Layer", "Path", "PrimSpecifierKind"]
