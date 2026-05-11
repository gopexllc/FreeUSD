"""UsdGeom-shaped schema helpers (tokens plus ``Boundable``/``Imageable``/``Xformable`` runtime subsets)."""

from __future__ import annotations

from importlib import import_module

from . import tokens

_geom = import_module("freeusd._native").usdGeom

Boundable = _geom.Boundable
Imageable = _geom.Imageable
Xformable = _geom.Xformable


def token_mesh():
    return tokens.Mesh()


def token_xform():
    return tokens.Xform()


def token_scope():
    return tokens.Scope()


__all__ = ["tokens", "Boundable", "Imageable", "Xformable", "token_mesh", "token_xform", "token_scope"]
