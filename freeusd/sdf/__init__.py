"""Sdf-shaped scene description (paths, layers)."""

from __future__ import annotations

import importlib

from . import builtin_tokens

_sdf = importlib.import_module("freeusd._native").sdf

AssetPath = _sdf.AssetPath
Layer = _sdf.Layer
LayerOffset = _sdf.LayerOffset
Path = _sdf.Path
PrimReference = _sdf.PrimReference
PrimSpecifierKind = _sdf.PrimSpecifierKind

__all__ = [
    "AssetPath",
    "Layer",
    "LayerOffset",
    "Path",
    "PrimReference",
    "PrimSpecifierKind",
    "builtin_tokens",
]
