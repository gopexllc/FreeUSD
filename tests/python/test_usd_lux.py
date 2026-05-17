"""UsdLux DistantLight vs parity_lux_distant.usda fixture."""

from __future__ import annotations

from pathlib import Path

from freeusd.sdf import Path as SdfPath
from freeusd.usd import RootLayerSublayersPolicy, Stage
from freeusd.usdLux import DistantLight

FIXTURES = Path(__file__).resolve().parents[1] / "fixtures"


def test_parity_lux_distant_fixture() -> None:
    stage = Stage.open_from_root_file(
        str(FIXTURES / "parity_lux_distant.usda"), RootLayerSublayersPolicy.depth_first
    )
    assert stage is not None

    sun = DistantLight.read_from_prim(stage, SdfPath.from_string("/World/Sun"))
    assert sun
    assert abs(sun.get_intensity(1.0) - 1200.0) < 1e-5
    color = sun.get_color(1.0)
    assert color is not None
    assert abs(color.x() - 1.0) < 1e-5
    assert abs(color.y() - 0.95) < 1e-5
    assert abs(color.z() - 0.8) < 1e-5
    assert abs(sun.get_angle(1.0) - 0.53) < 1e-5
