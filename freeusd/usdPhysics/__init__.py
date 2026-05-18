"""UsdPhysics-shaped schema helpers (tokens plus minimal PhysicsScene runtime)."""

from __future__ import annotations

from importlib import import_module

from . import tokens

_physics = import_module("freeusd._native").usdPhysics

PhysicsScene = _physics.PhysicsScene

__all__ = [
    "PhysicsScene",
    "tokens",
]
