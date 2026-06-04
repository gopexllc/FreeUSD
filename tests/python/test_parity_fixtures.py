from __future__ import annotations

from pathlib import Path

from freeusd.sdf import Path as SdfPath
from freeusd.tf import Token
from freeusd.usd import RootLayerSublayersPolicy, Stage
from freeusd.usd.crate import (
    read_usdc_fields_table_from_path,
    read_usdc_specs_table_from_path,
    read_usdc_fieldsets_table_from_path,
    read_usdc_values_table_from_path,
    read_usdc_typed_values_table_from_path,
    read_usdc_path_table_from_path,
    read_usdc_string_table_from_path,
    read_usdc_token_table_from_path,
)
from freeusd.usdGeom import Boundable, Imageable
from freeusd.usdUtils import flatten_stage_at_time


FIXTURES = Path(__file__).resolve().parents[1] / "fixtures"


def test_parity_stack_fixture_drives_time_remap_and_flatten() -> None:
    stage = Stage.open_from_root_file(str(FIXTURES / "parity_stack_root.usda"), RootLayerSublayersPolicy.depth_first)
    assert stage is not None
    model = SdfPath.from_string("/World/Model")
    assert stage.read_field_double(model, Token("stackedOnly"), 15.0) == 50.0
    assert stage.read_field_double(model, Token("stackedOnly"), -5.0) == 50.0
    assert stage.list_composed_field_sample_times(model, Token("stackedOnly")) == [-5.0, 15.0]

    flat = flatten_stage_at_time(stage, 15.0)
    assert flat.list_sample_times(model, Token("stackedOnly")) == [-5.0, 15.0]
    assert flat.get_field(model, Token("stackedOnly")).as_double() == 50.0


def test_parity_namespace_fixture_executes_references_and_payloads() -> None:
    stage = Stage.open_from_root_file(str(FIXTURES / "parity_namespace.usda"), RootLayerSublayersPolicy.depth_first)
    assert stage is not None
    published = SdfPath.from_string("/Library/Published")
    ref_leaf = SdfPath.from_string("/Library/Published/RefLeaf")
    payload_leaf = SdfPath.from_string("/Library/Published/PayloadLeaf")

    assert stage.prim_at(ref_leaf).is_valid()
    assert stage.prim_at(payload_leaf).is_valid()
    assert stage.read_field_double(published, Token("refOnly"), 1.0) == 11.0
    assert stage.read_field_double(ref_leaf, Token("branch"), 1.0) == 22.0
    assert stage.read_field_double(published, Token("payloadOnly"), 1.0) == 33.0
    assert stage.read_field_double(payload_leaf, Token("branch"), 1.0) == 44.0


def test_parity_variant_fixture_executes_selected_variant_payload() -> None:
    stage = Stage.open_from_root_file(str(FIXTURES / "parity_variants.usda"), RootLayerSublayersPolicy.depth_first)
    assert stage is not None
    host = SdfPath.from_string("/World/VariantHost")
    child = SdfPath.from_string("/World/VariantHost/VariantChild")
    assert stage.prim_at(child).is_valid()
    assert stage.read_field_double(host, Token("variantValue"), 1.0) == 5.0
    assert stage.read_field_double(child, Token("branch"), 1.0) == 9.0


def test_parity_tables_fixture_decodes_structured_usdc_tables() -> None:
    usdc = str(FIXTURES / "parity_tables.usdc")
    ok, tokens, err = read_usdc_token_table_from_path(usdc, 8, 1024)
    assert ok and err == ""
    assert tokens == ["render", "invisible"]

    ok, strings, err = read_usdc_string_table_from_path(usdc, 8, 1024)
    assert ok and err == ""
    assert strings == ["hello", "world"]

    ok, paths, err = read_usdc_path_table_from_path(usdc, 8, 1024)
    assert ok and err == ""
    assert [p.text() for p in paths] == ["/World", "/World/Cube"]

    ok, fields, err = read_usdc_fields_table_from_path(usdc, 8, 1024)
    assert ok and err == ""
    assert fields == [
        {"token_index": 0, "value_type_token_index": 1},
        {"token_index": 1, "value_type_token_index": 0},
    ]

    ok, specs, err = read_usdc_specs_table_from_path(usdc, 8, 1024)
    assert ok and err == ""
    assert specs == [
        {"path_index": 0, "field_set_index": 0, "spec_type": 1},
        {"path_index": 1, "field_set_index": 1, "spec_type": 2},
    ]

    ok, fieldsets, err = read_usdc_fieldsets_table_from_path(usdc, 8, 8, 1024)
    assert ok and err == ""
    assert fieldsets == [[0, 1], [1]]

    ok, typed, err = read_usdc_typed_values_table_from_path(usdc, 16, 1024)
    assert ok and err == ""
    assert typed[0]["kind"] == 1 and typed[0]["int32_value"] == 42
    assert typed[1]["kind"] == 2 and abs(typed[1]["float_value"] - 1.5) < 1e-5
    assert typed[2]["kind"] == 3 and typed[2]["token_index"] == 0
    assert typed[3]["kind"] == 4 and typed[3]["bool_value"] is True
    assert len(typed) == 13
    assert typed[9]["kind"] == 10 and abs(typed[9]["vec3d_value"][2] - 6.0) < 1e-9
    assert typed[10]["kind"] == 11 and typed[10]["int32_array"] == [7, 8, 9]
    assert typed[11]["kind"] == 12 and typed[11]["float_array"] == [0.25, 0.75]
    assert typed[12]["kind"] == 13 and typed[12]["double_array"] == [1.0, 2.0]

    ok, values, err = read_usdc_values_table_from_path(usdc, 16, 1024)
    assert ok and err == ""
    assert len(values) == 13


def test_parity_geom_mesh_fixture_reads_points() -> None:
    from freeusd.sdf import Path as SdfPath
    from freeusd.usd import RootLayerSublayersPolicy, Stage
    from freeusd.usdGeom import Mesh

    stage = Stage.open_from_root_file(str(FIXTURES / "parity_geom_mesh.usda"), RootLayerSublayersPolicy.none)
    assert stage is not None
    mesh = Mesh(stage.prim_at(SdfPath.from_string("/World/Tri")))
    points = mesh.get_points(1.0)
    assert len(points) == 3
    counts = mesh.get_face_vertex_counts(1.0)
    assert counts == [3]
    assert mesh.get_face_vertex_indices(1.0) == [0, 1, 2]
    colors = mesh.get_display_color(1.0)
    assert len(colors) == 3
    normals = mesh.get_normals(1.0)
    assert len(normals) == 3
    assert all(abs(n.z() - 1.0) < 1e-5 for n in normals)
    st = mesh.get_primvars_st(1.0)
    assert len(st) == 3
    assert abs(st[1].s - 1.0) < 1e-5
    assert mesh.get_display_opacity(1.0) == 0.75


def test_parity_shade_texture_fixture_reads_asset_path() -> None:
    from freeusd.sdf import Path as SdfPath
    from freeusd.usd import RootLayerSublayersPolicy, Stage
    from freeusd.usdShade import Material, PreviewSurface

    stage = Stage.open_from_root_file(str(FIXTURES / "parity_shade_texture.usda"), RootLayerSublayersPolicy.none)
    assert stage is not None
    mat = Material.read_from_prim(stage, SdfPath.from_string("/World/Looks/Material"))
    preview = PreviewSurface.read_from_prim(stage, mat.get_surface_shader_path())
    assert preview.get_diffuse_texture_asset_path(1.0) == "textures/albedo.png"


def test_parity_lux_sphere_fixture_reads_inputs() -> None:
    from freeusd.sdf import Path as SdfPath
    from freeusd.usd import RootLayerSublayersPolicy, Stage
    from freeusd.usdLux import SphereLight

    stage = Stage.open_from_root_file(str(FIXTURES / "parity_lux_sphere.usda"), RootLayerSublayersPolicy.none)
    assert stage is not None
    light = SphereLight.read_from_prim(stage, SdfPath.from_string("/World/Bulb"))
    assert light.get_radius(1.0) == 0.25


def test_parity_shade_pbr_textures_fixture() -> None:
    from freeusd.sdf import Path as SdfPath
    from freeusd.usd import RootLayerSublayersPolicy, Stage
    from freeusd.usdShade import PreviewSurface

    stage = Stage.open_from_root_file(
        str(FIXTURES / "parity_shade_pbr_textures.usda"), RootLayerSublayersPolicy.none
    )
    assert stage is not None
    preview = PreviewSurface.read_from_prim(stage, SdfPath.from_string("/World/Looks/Material/PreviewSurface"))
    assert preview.get_normal_texture_asset_path(1.0) == "textures/normal.png"
    assert preview.get_roughness_texture_asset_path(1.0) == "textures/roughness.png"


def test_parity_lux_disk_fixture_reads_inputs() -> None:
    from freeusd.sdf import Path as SdfPath
    from freeusd.usd import RootLayerSublayersPolicy, Stage
    from freeusd.usdLux import DiskLight

    stage = Stage.open_from_root_file(str(FIXTURES / "parity_lux_disk.usda"), RootLayerSublayersPolicy.none)
    assert stage is not None
    light = DiskLight.read_from_prim(stage, SdfPath.from_string("/World/Softbox"))
    assert light.get_radius(1.0) == 0.75


def test_parity_lux_dome_fixture_reads_inputs() -> None:
    from freeusd.sdf import Path as SdfPath
    from freeusd.usd import RootLayerSublayersPolicy, Stage
    from freeusd.usdLux import DomeLight

    stage = Stage.open_from_root_file(str(FIXTURES / "parity_lux_dome.usda"), RootLayerSublayersPolicy.none)
    assert stage is not None
    light = DomeLight.read_from_prim(stage, SdfPath.from_string("/World/Sky"))
    assert light.get_texture_file_asset_path(1.0) == "textures/sky.hdr"
    assert light.get_texture_format(1.0) == "latlong"


def test_parity_lux_cylinder_fixture_reads_inputs() -> None:
    from freeusd.sdf import Path as SdfPath
    from freeusd.usd import RootLayerSublayersPolicy, Stage
    from freeusd.usdLux import CylinderLight

    stage = Stage.open_from_root_file(str(FIXTURES / "parity_lux_cylinder.usda"), RootLayerSublayersPolicy.none)
    assert stage is not None
    light = CylinderLight.read_from_prim(stage, SdfPath.from_string("/World/Tube"))
    assert light.get_length(1.0) == 2.5


def test_parity_lux_rect_fixture_reads_inputs() -> None:
    from freeusd.sdf import Path as SdfPath
    from freeusd.usd import RootLayerSublayersPolicy, Stage
    from freeusd.usdLux import RectLight

    stage = Stage.open_from_root_file(str(FIXTURES / "parity_lux_rect.usda"), RootLayerSublayersPolicy.none)
    assert stage is not None
    light = RectLight.read_from_prim(stage, SdfPath.from_string("/World/Panel"))
    assert light.get_width(1.0) == 2.0
    assert light.get_height(1.0) == 1.0


def test_parity_physics_rigid_body_fixture_reads_mass() -> None:
    from freeusd.sdf import Path as SdfPath
    from freeusd.usd import RootLayerSublayersPolicy, Stage
    from freeusd.usdPhysics import RigidBodyAPI

    stage = Stage.open_from_root_file(str(FIXTURES / "parity_physics_rigid_body.usda"), RootLayerSublayersPolicy.none)
    assert stage is not None
    body = RigidBodyAPI.read_from_prim(stage, SdfPath.from_string("/World/Body"))
    assert body.is_rigid_body_api()
    assert body.get_mass(1.0) == 2.5


def test_parity_physics_fixed_joint_fixture_reads_body_rels() -> None:
    from freeusd.sdf import Path as SdfPath
    from freeusd.usdPhysics import FixedJoint

    stage = Stage.open_from_root_file(str(FIXTURES / "parity_physics_fixed_joint.usda"), RootLayerSublayersPolicy.none)
    assert stage is not None
    joint = FixedJoint.read_from_prim(stage, SdfPath.from_string("/World/Anchor"))
    assert joint.is_fixed_joint()
    assert joint.get_body0().text() == "/World/BodyA"
    assert joint.get_body1().text() == "/World/BodyB"
    assert joint.get_joint_enabled(1.0) is True


def test_parity_physics_mass_fixture_reads_density_and_center_of_mass() -> None:
    from freeusd.gf import Vec3f
    from freeusd.sdf import Path as SdfPath
    from freeusd.usdPhysics import MassAPI

    stage = Stage.open_from_root_file(str(FIXTURES / "parity_physics_mass.usda"), RootLayerSublayersPolicy.none)
    assert stage is not None
    prop = MassAPI.read_from_prim(stage, SdfPath.from_string("/World/Prop"))
    assert prop.is_mass_api()
    assert prop.get_density(1.0) == 2.0
    com = prop.get_center_of_mass(1.0)
    assert isinstance(com, Vec3f)
    assert abs(com.y() - 0.5) < 1e-5


def test_parity_physics_rigid_body_kinematic_fixture_reads_kinematic_enabled() -> None:
    from freeusd.sdf import Path as SdfPath
    from freeusd.usdPhysics import RigidBodyAPI

    stage = Stage.open_from_root_file(
        str(FIXTURES / "parity_physics_rigid_body_kinematic.usda"), RootLayerSublayersPolicy.none
    )
    assert stage is not None
    body = RigidBodyAPI.read_from_prim(stage, SdfPath.from_string("/World/Body"))
    assert body
    assert body.is_rigid_body_api()
    assert body.get_kinematic_enabled(1.0) is True


def test_parity_physics_rigid_body_refs_fixture_composes_mass() -> None:
    from freeusd.sdf import Path as SdfPath
    from freeusd.usdPhysics import RigidBodyAPI

    stage = Stage.open_from_root_file(str(FIXTURES / "parity_physics_rigid_body_refs.usda"), RootLayerSublayersPolicy.none)
    assert stage is not None
    body = RigidBodyAPI.read_from_prim(stage, SdfPath.from_string("/World/RefHost"))
    assert body.is_rigid_body_api()
    assert body.get_mass(1.0) == 7.0


def test_parity_physics_rigid_body_inherit_fixture_composes_api_schemas() -> None:
    from freeusd.sdf import Path as SdfPath
    from freeusd.tf import Token
    from freeusd.usd import RootLayerSublayersPolicy, Stage
    from freeusd.usdPhysics import RigidBodyAPI, tokens

    stage = Stage.open_from_root_file(
        str(FIXTURES / "parity_physics_rigid_body_inherit.usda"), RootLayerSublayersPolicy.none
    )
    assert stage is not None
    body_path = SdfPath.from_string("/World/Body")
    body = RigidBodyAPI.read_from_prim(stage, body_path)
    assert body.is_rigid_body_api()
    assert body.get_mass(1.0) == 4.0
    schemas = stage.read_field_token_array(body_path, Token("apiSchemas"), 1.0)
    assert [t.text() for t in schemas] == [tokens.PhysicsRigidBodyAPI().text()]


def test_parity_physics_scene_fixture_reads_gravity() -> None:
    from freeusd.sdf import Path as SdfPath
    from freeusd.usd import RootLayerSublayersPolicy, Stage
    from freeusd.usdPhysics import PhysicsScene

    stage = Stage.open_from_root_file(str(FIXTURES / "parity_physics_scene.usda"), RootLayerSublayersPolicy.none)
    assert stage is not None
    scene = PhysicsScene.read_from_prim(stage, SdfPath.from_string("/World/Physics"))
    assert scene.is_physics_scene()
    gravity = scene.get_gravity_direction(1.0)
    assert gravity is not None
    assert gravity.as_array() == [0.0, 0.0, -1.0]
    assert scene.get_gravity_magnitude(1.0) == 981.0


def test_parity_vol_volume_fixture_reads_field_relationship() -> None:
    from freeusd.sdf import Path as SdfPath
    from freeusd.usd import RootLayerSublayersPolicy, Stage
    from freeusd.usdVol import Volume

    stage = Stage.open_from_root_file(str(FIXTURES / "parity_vol_volume.usda"), RootLayerSublayersPolicy.none)
    assert stage is not None
    cloud = Volume.read_from_prim(stage, SdfPath.from_string("/World/Cloud"))
    assert cloud.is_volume()
    targets = cloud.get_field_relationship_targets()
    assert [path.text() for path in targets] == ["/World/Cloud/Smoke"]
    fields = cloud.get_open_vdb_field_assets()
    assert len(fields) == 1
    assert fields[0].is_open_vdb_asset()
    assert fields[0].get_file_path(1.0) == "volumes/cloud/smoke.vdb"
    assert fields[0].get_field_name(1.0) == "density"


def test_parity_vol_openvdb_fixture_reads_field_asset() -> None:
    from freeusd.sdf import Path as SdfPath
    from freeusd.usd import RootLayerSublayersPolicy, Stage
    from freeusd.usdVol import OpenVDBAsset

    stage = Stage.open_from_root_file(str(FIXTURES / "parity_vol_openvdb.usda"), RootLayerSublayersPolicy.none)
    assert stage is not None
    field = OpenVDBAsset.read_from_prim(stage, SdfPath.from_string("/World/Smoke"))
    assert field.is_open_vdb_asset()
    assert field.get_file_path(1.0) == "volumes/smoke.vdb"
    assert field.get_field_name(1.0) == "density"


def test_parity_specializes_fixture_composes_field_reads() -> None:
    stage = Stage.open_from_root_file(str(FIXTURES / "parity_specializes.usda"), RootLayerSublayersPolicy.depth_first)
    assert stage is not None
    host = SdfPath.from_string("/World/Host")
    base = SdfPath.from_string("/World/BaseSpec")
    prim = stage.prim_at(host)
    assert prim.has_specializes()
    assert stage.read_prim_specializes(host)[0] == base
    assert stage.read_field_double(host, Token("sharedStrength"), 1.0) == 10.0
    assert stage.read_field_double(host, Token("fromSpec"), 1.0) == 99.0
    assert stage.read_field_double(host, Token("hostOnly"), 1.0) == 3.0
    assert "sharedStrength" in stage.list_composed_field_names(host)
    assert not stage.has_field_opinion(base, Token("hostOnly"))


def test_parity_imageable_fixture_is_primary_scene_anchor() -> None:
    stage = Stage.open_from_root_file(str(FIXTURES / "parity_imageable.usda"), RootLayerSublayersPolicy.depth_first)
    assert stage is not None
    cube_prim = stage.prim_at(SdfPath.from_string("/World/Cube"))
    cube = Imageable(cube_prim)
    assert cube.compute_purpose(1.0) == "render"
    assert cube.compute_visibility(1.0) is False
    world = Boundable(cube_prim).compute_world_bound(1.0)
    assert world.min.as_array() == [0.0, 1.0, 2.0]
    assert world.max.as_array() == [2.0, 3.0, 4.0]
