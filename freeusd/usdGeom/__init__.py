"""UsdGeom-shaped schema helpers (minimal)."""

from __future__ import annotations

import importlib

_geom = importlib.import_module("freeusd._native").usdGeom


def token_mesh():
    return _geom.token_mesh()


def token_xform():
    return _geom.token_xform()


def token_scope():
    return _geom.token_scope()


__all__ = ["token_mesh", "token_xform", "token_scope"]
