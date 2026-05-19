"""UsdPhysics-shaped schema helpers (tokens plus minimal PhysicsScene and RigidBodyAPI runtime)."""

from __future__ import annotations

from importlib import import_module

from . import tokens

_physics = import_module("freeusd._native").usdPhysics

PhysicsScene = _physics.PhysicsScene
RigidBodyAPI = _physics.RigidBodyAPI

__all__ = [
    "PhysicsScene",
    "RigidBodyAPI",
    "tokens",
]
