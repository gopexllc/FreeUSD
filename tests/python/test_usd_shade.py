"""UsdShade helpers vs parity_shade_preview.usda fixture."""

from __future__ import annotations

from pathlib import Path

from freeusd.sdf import Path as SdfPath
from freeusd.tf import Token
from freeusd.usd import RootLayerSublayersPolicy, Stage
from freeusd.usdShade import Material, PreviewSurface, Shader

FIXTURES = Path(__file__).resolve().parents[1] / "fixtures"


def test_parity_shade_preview_fixture() -> None:
    stage = Stage.open_from_root_file(
        str(FIXTURES / "parity_shade_preview.usda"), RootLayerSublayersPolicy.depth_first
    )
    assert stage is not None

    mat = Material.read_from_prim(stage, SdfPath.from_string("/World/Looks/Material"))
    assert mat
    surface = mat.get_surface_shader_path()
    assert surface == SdfPath.from_string("/World/Looks/Material/PreviewSurface")

    shaders = mat.list_shader_prim_paths()
    assert len(shaders) == 1
    assert shaders[0] == surface

    shader = Shader.read_from_prim(stage, surface)
    assert shader
    assert shader.get_shader_id().text() == "UsdPreviewSurface"

    diffuse = shader.get_diffuse_color(1.0)
    assert diffuse is not None
    assert abs(diffuse.x() - 0.8) < 1e-5
    assert abs(diffuse.y() - 0.2) < 1e-5
    assert abs(diffuse.z() - 0.1) < 1e-5
    assert abs(shader.get_metallic(1.0) - 0.5) < 1e-5
    assert abs(shader.get_roughness(1.0) - 0.3) < 1e-5
    assert abs(shader.get_opacity(1.0) - 1.0) < 1e-5

    preview = PreviewSurface.read_from_prim(stage, surface)
    assert preview
    assert preview.is_preview_surface()
    pv_diffuse = preview.get_diffuse_color(1.0)
    assert pv_diffuse is not None
    assert abs(pv_diffuse.x() - 0.8) < 1e-5

    mesh = stage.prim_at(SdfPath.from_string("/World/Cube"))
    bindings = mesh.get_relationship_targets(Token("material:binding"))
    assert len(bindings) == 1
    assert bindings[0] == SdfPath.from_string("/World/Looks/Material")
