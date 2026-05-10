"""UsdGeom-shaped schema helpers (tokens + ``Xformable`` xform stack subset)."""

from __future__ import annotations

from importlib import import_module

from . import tokens

_geom = import_module("freeusd._native").usdGeom

Xformable = _geom.Xformable


def token_mesh():
    return tokens.Mesh()


def token_xform():
    return tokens.Xform()


def token_scope():
    return tokens.Scope()


__all__ = ["tokens", "Xformable", "token_mesh", "token_xform", "token_scope"]
