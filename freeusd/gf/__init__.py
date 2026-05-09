"""Gf-shaped linear algebra types (minimal containers; clean-room)."""

from __future__ import annotations

from importlib import import_module

_gf = import_module("freeusd._native").gf

BBox3d = _gf.BBox3d
Matrix4d = _gf.Matrix4d
Quatd = _gf.Quatd
Range1d = _gf.Range1d
Vec3d = _gf.Vec3d

__all__ = ["BBox3d", "Matrix4d", "Quatd", "Range1d", "Vec3d"]
