"""UsdPhysics-shaped schema token stubs."""

from __future__ import annotations

from importlib import import_module

_m = import_module("freeusd._native").usdPhysics.tokens

Scene = _m.Scene
RigidBodyAPI = _m.RigidBodyAPI
CollisionAPI = _m.CollisionAPI

__all__ = ["Scene", "RigidBodyAPI", "CollisionAPI"]
