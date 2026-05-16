"""UsdSkel-shaped schema helpers (tokens plus minimal glTF mapping runtime)."""

from __future__ import annotations

from importlib import import_module

from . import tokens
from .gltf_mapping import accumulate_world_transforms, build_joint_parent_indices, compute_world_bind_matrices

_skel = import_module("freeusd._native").usdSkel

Skeleton = _skel.Skeleton
SkelAnimation = _skel.SkelAnimation
SkelBinding = _skel.SkelBinding
SkelRoot = _skel.SkelRoot
JointTransform = _skel.JointTransform

__all__ = [
    "JointTransform",
    "Skeleton",
    "SkelAnimation",
    "SkelBinding",
    "SkelRoot",
    "accumulate_world_transforms",
    "build_joint_parent_indices",
    "compute_world_bind_matrices",
    "tokens",
]
