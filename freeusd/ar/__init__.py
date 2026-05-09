"""Ar-shaped asset resolution."""

from __future__ import annotations

import importlib

_ar = importlib.import_module("freeusd._native").ar

DefaultResolver = _ar.DefaultResolver

__all__ = ["DefaultResolver"]
