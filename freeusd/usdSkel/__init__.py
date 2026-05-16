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
BlendShape = _skel.BlendShape
SkelBlendShapes = _skel.SkelBlendShapes
MorphTargetBinding = _skel.MorphTargetBinding
MorphTargets = _skel.MorphTargets
JointTransform = _skel.JointTransform
apply_morph_targets_to_points = _skel.apply_morph_targets_to_points
compute_skinning_matrices = _skel.compute_skinning_matrices
deform_points_with_skeleton = _skel.deform_points_with_skeleton
build_joint_world_matrices_from_animation = _skel.build_joint_world_matrices_from_animation

__all__ = [
    "BlendShape",
    "JointTransform",
    "MorphTargetBinding",
    "MorphTargets",
    "apply_morph_targets_to_points",
    "build_joint_world_matrices_from_animation",
    "compute_skinning_matrices",
    "deform_points_with_skeleton",
    "Skeleton",
    "SkelAnimation",
    "SkelBinding",
    "SkelBlendShapes",
    "SkelRoot",
    "accumulate_world_transforms",
    "build_joint_parent_indices",
    "compute_world_bind_matrices",
    "tokens",
]
