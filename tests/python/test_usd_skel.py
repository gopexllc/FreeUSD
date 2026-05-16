"""UsdSkel helpers vs glTF-shaped parity_skel_gltf.usda fixture."""

from __future__ import annotations

from pathlib import Path

from freeusd.gf import Matrix4d, Vec3f
from freeusd.sdf import Path as SdfPath
from freeusd.usd import RootLayerSublayersPolicy, Stage
from freeusd.usdSkel import (
    MorphTargets,
    SkelAnimation,
    SkelBinding,
    SkelRoot,
    Skeleton,
    build_joint_world_matrices_from_animation,
    deform_points_with_skeleton,
)
from freeusd.usdSkel.gltf_mapping import build_joint_parent_indices


# glTF mapping: joints -> skin.joints; translations/rotations/scales -> animation TRS channels
FIXTURES = Path(__file__).resolve().parents[1] / "fixtures"


def test_parity_skel_gltf_fixture_matches_gltf_channel_shape() -> None:
    stage = Stage.open_from_root_file(
        str(FIXTURES / "parity_skel_gltf.usda"), RootLayerSublayersPolicy.depth_first
    )
    assert stage is not None

    skel_prim = stage.prim_at(SdfPath.from_string("/World/Character/Skeleton"))
    skel = Skeleton(skel_prim)
    joints = skel.get_joint_names()
    assert joints == ["Root", "Root/Hip"]
    parents = skel.get_joint_parent_indices()
    assert parents == [-1, 0]

    def mat_ty(m: object) -> float:
        if hasattr(m, "as_list"):
            return float(m.as_list()[13])  # type: ignore[attr-defined]
        return float(m[13])  # type: ignore[index]

    bind = skel.get_bind_transforms(1.0)
    assert len(bind) == 2
    assert abs(mat_ty(bind[1]) - 1.0) < 1e-5

    world = skel.compute_world_bind_matrices(1.0)
    assert len(world) == 2
    assert abs(mat_ty(world[1]) - 1.0) < 1e-5

    anim_prim = stage.prim_at(SdfPath.from_string("/World/Character/Anim"))
    anim = SkelAnimation(anim_prim)
    assert anim.list_translation_sample_times() == [0.0, 1.0]

    t0 = anim.get_translations(0.0)
    t1 = anim.get_translations(1.0)
    assert len(t0) == 2
    def vec3_y(v: object) -> float:
        if hasattr(v, "y"):
            return float(v.y())  # type: ignore[attr-defined]
        return float(v[1])  # type: ignore[index]

    assert abs(vec3_y(t0[1]) - 1.0) < 1e-5
    assert abs(vec3_y(t1[1]) - 2.0) < 1e-5

    rot1 = anim.get_rotations(1.0)
    rot_w = rot1[0].real if hasattr(rot1[0], "real") else rot1[0][0]
    assert abs(rot_w - 0.7071068) < 1e-4

    assert build_joint_parent_indices(joints) == parents


def test_parity_skel_binding_fixture_resolves_root_and_primvars() -> None:
    stage = Stage.open_from_root_file(
        str(FIXTURES / "parity_skel_binding.usda"), RootLayerSublayersPolicy.depth_first
    )
    assert stage is not None

    root = SkelRoot.read_from_prim(stage, SdfPath.from_string("/World/SkelCharacter"))
    assert root
    skel_path = root.find_skeleton_path()
    assert skel_path is not None
    assert skel_path.text() == "/World/SkelCharacter/Skeleton"

    anim_path = root.get_animation_source_path()
    assert anim_path is not None
    assert anim_path.text() == "/World/SkelCharacter/Anim"

    skel = root.get_skeleton()
    assert skel.get_joint_names() == ["Root", "Root/Hip"]
    assert root.get_animation_source()

    body = stage.prim_at(SdfPath.from_string("/World/SkelCharacter/Body"))
    binding = SkelBinding.read_from_geom_prim(body)
    assert binding
    assert binding.skeleton_path.text() == "/World/SkelCharacter/Skeleton"

    indices = SkelBinding.read_joint_indices(body)
    weights = SkelBinding.read_joint_weights(body)
    assert indices is not None and weights is not None
    assert len(indices) == len(weights) == 8
    assert SkelBinding.validate_influence_counts(indices, weights, 4)
    assert indices[0] == 0 and indices[1] == 1
    assert abs(weights[0] - 1.0) < 1e-5
    assert abs(weights[4] - 0.5) < 1e-5


def test_parity_skel_blend_shapes_fixture_applies_morph_weights() -> None:
    stage = Stage.open_from_root_file(
        str(FIXTURES / "parity_skel_blend_shapes.usda"), RootLayerSublayersPolicy.depth_first
    )
    assert stage is not None

    face = stage.prim_at(SdfPath.from_string("/World/Face"))
    morphs = MorphTargets.read_from_geom_prim(face)
    assert morphs
    assert morphs.get_blend_shape_tokens() == ["Smile", "Blink"]

    weights = morphs.get_weights(1.0)
    assert weights is not None
    assert len(weights) == 2
    assert abs(weights[0] - 1.0) < 1e-5
    assert abs(weights[1] - 0.5) < 1e-5

    def vec3_y(v: object) -> float:
        if hasattr(v, "y"):
            return float(v.y())  # type: ignore[attr-defined]
        return float(v[1])  # type: ignore[index]

    def vec3_z(v: object) -> float:
        if hasattr(v, "z"):
            return float(v.z())  # type: ignore[attr-defined]
        return float(v[2])  # type: ignore[index]

    morphed = morphs.evaluate_points(1.0)
    assert morphed is not None
    assert len(morphed) == 2
    assert abs(vec3_y(morphed[0]) - 1.0) < 1e-5
    assert abs(vec3_z(morphed[0]) - 0.5) < 1e-5
    assert abs(vec3_y(morphed[1]) - 1.0) < 1e-5


def test_parity_skel_skinning_fixture_deforms_point() -> None:
    stage = Stage.open_from_root_file(
        str(FIXTURES / "parity_skel_skinning.usda"), RootLayerSublayersPolicy.depth_first
    )
    assert stage is not None

    body = stage.prim_at(SdfPath.from_string("/World/SkelCharacter/Body"))
    skel = Skeleton(stage.prim_at(SdfPath.from_string("/World/SkelCharacter/Skeleton")))
    anim = SkelAnimation(stage.prim_at(SdfPath.from_string("/World/SkelCharacter/Anim")))

    points = [Vec3f(0.0, 1.0, 0.0)]

    indices = SkelBinding.read_joint_indices(body, 1.0)
    weights = SkelBinding.read_joint_weights(body, 1.0)
    assert indices is not None and weights is not None

    bind = skel.get_bind_transforms(1.0)
    assert bind is not None
    joint_world = build_joint_world_matrices_from_animation(skel, anim, 1.0)
    assert len(joint_world) == 2

    deformed = deform_points_with_skeleton(points, indices, weights, 1, joint_world, bind)
    assert deformed is not None
    assert deformed[0][1] > points[0].y()
