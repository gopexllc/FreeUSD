"""UsdSkel blend shapes / morph targets vs parity_skel_blend_shapes.usda."""

from __future__ import annotations

from pathlib import Path

from freeusd.sdf import Path as SdfPath
from freeusd.usd import RootLayerSublayersPolicy, Stage
from freeusd.usdSkel import BlendShape, MorphTargets, SkelBlendShapes


FIXTURES = Path(__file__).resolve().parents[1] / "fixtures"


def test_blend_shapes_fixture_morphs_points() -> None:
    stage = Stage.open_from_root_file(
        str(FIXTURES / "parity_skel_blend_shapes.usda"), RootLayerSublayersPolicy.depth_first
    )
    assert stage is not None

    face = stage.prim_at(SdfPath.from_string("/World/Face"))
    binding = SkelBlendShapes.read_from_geom_prim(face)
    assert binding.blend_shape_tokens == ["Smile", "Blink"]

    smile = BlendShape(stage.prim_at(SdfPath.from_string("/World/BlendShapes/Smile")))
    offsets = smile.get_offsets(1.0)
    assert offsets is not None and len(offsets) == 2

    weights = binding.get_weights(1.0)
    assert weights == [1.0, 0.5]

    morph = MorphTargets.read_from_geom_prim(face)
    morphed = morph.evaluate_points(1.0)
    assert morphed is not None
    assert abs(morphed[0][1] - 1.0) < 1e-5
    assert abs(morphed[0][2] - 0.5) < 1e-5
    assert abs(morphed[1][1] - 1.0) < 1e-5
