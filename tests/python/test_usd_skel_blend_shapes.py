"""UsdSkel blend shapes / morph targets vs parity_skel_blend_shapes.usda."""

from __future__ import annotations

from pathlib import Path

from freeusd.sdf import Path as SdfPath
from freeusd.usd import RootLayerSublayersPolicy, Stage
from freeusd.usdSkel import BlendShape, MorphTargets, SkelBlendShapes


FIXTURES = Path(__file__).resolve().parents[1] / "fixtures"


def _vec3_y(v: object) -> float:
    if hasattr(v, "y"):
        return float(v.y())  # type: ignore[attr-defined]
    return float(v[1])  # type: ignore[index]


def test_blend_shapes_fixture_morphs_points() -> None:
    stage = Stage.open_from_root_file(
        str(FIXTURES / "parity_skel_blend_shapes.usda"), RootLayerSublayersPolicy.depth_first
    )
    assert stage is not None

    face = stage.prim_at(SdfPath.from_string("/World/FaceRig/Face"))
    binding = SkelBlendShapes.read_from_geom_prim(face)
    assert binding
    assert binding.blend_shape_tokens == ["smile", "blink"]

    smile = BlendShape(stage.prim_at(SdfPath.from_string("/World/FaceRig/Smile")))
    offsets = smile.get_offsets(1.0)
    assert offsets is not None
    assert len(offsets) == 4
    assert abs(_vec3_y(offsets[0]) - 0.5) < 1e-5

    w0 = binding.get_weights(0.0)
    w1 = binding.get_weights(1.0)
    w2 = binding.get_weights(2.0)
    assert w0 is not None and w1 is not None and w2 is not None
    assert w0 == [0.0, 0.0]
    assert w1 == [1.0, 0.0]
    assert w2 == [1.0, 1.0]

    morph = MorphTargets.read_from_geom_prim(face)
    assert morph.evaluate_points(0.0) is not None
    p0 = morph.evaluate_points(0.0)
    assert p0 is not None and abs(_vec3_y(p0[0])) < 1e-5

    p1 = morph.evaluate_points(1.0)
    assert p1 is not None and abs(_vec3_y(p1[0]) - 0.5) < 1e-5

    p2 = morph.evaluate_points(2.0)
    assert p2 is not None and abs(_vec3_y(p2[2]) - 0.75) < 1e-5
