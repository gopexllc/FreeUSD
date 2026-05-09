"""Ar-shaped asset resolution."""

from __future__ import annotations

import importlib

_ar = importlib.import_module("freeusd._native").ar

DefaultResolver = _ar.DefaultResolver
ResolvedPath = _ar.ResolvedPath

__all__ = ["DefaultResolver", "ResolvedPath"]
