"""UsdPhysics-shaped schema helpers (tokens plus minimal PhysicsScene, RigidBodyAPI, and CollisionAPI runtime)."""

from __future__ import annotations

from importlib import import_module

from . import tokens

_physics = import_module("freeusd._native").usdPhysics

PhysicsScene = _physics.PhysicsScene
RigidBodyAPI = _physics.RigidBodyAPI
CollisionAPI = _physics.CollisionAPI
MassAPI = _physics.MassAPI
FixedJoint = _physics.FixedJoint

__all__ = [
    "CollisionAPI",
    "FixedJoint",
    "MassAPI",
    "PhysicsScene",
    "RigidBodyAPI",
    "tokens",
]
