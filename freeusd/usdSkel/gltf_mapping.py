"""glTF-shaped joint hierarchy helpers (clean-room)."""

from __future__ import annotations

from importlib import import_module

_gltf = import_module("freeusd._native").usdSkel.gltf_mapping

build_joint_parent_indices = _gltf.build_joint_parent_indices
accumulate_world_transforms = _gltf.accumulate_world_transforms
compute_world_bind_matrices = _gltf.compute_world_bind_matrices

__all__ = [
    "accumulate_world_transforms",
    "build_joint_parent_indices",
    "compute_world_bind_matrices",
]
