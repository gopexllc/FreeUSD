#include <cassert>
#include <string>

#include "freeusd/sdf/path.hpp"
#include "freeusd/tf/token.hpp"
#include "freeusd/usd/crateFile.hpp"
#include "freeusd/usd/stage.hpp"
#include "freeusd/usdGeom/boundable.hpp"
#include "freeusd/usdGeom/imageable.hpp"
#include "freeusd/usdGeom/mesh.hpp"
#include "freeusd/usdSkel/skelAnimation.hpp"
#include "freeusd/usdSkel/skelBinding.hpp"
#include "freeusd/usdSkel/morphTargets.hpp"
#include "freeusd/usdSkel/skelRoot.hpp"
#include "freeusd/usdSkel/skeleton.hpp"
#include "freeusd/usdLux/cylinderLight.hpp"
#include "freeusd/usdLux/diskLight.hpp"
#include "freeusd/usdLux/domeLight.hpp"
#include "freeusd/usdLux/rectLight.hpp"
#include "freeusd/usdLux/sphereLight.hpp"
#include "freeusd/usdPhysics/collisionAPI.hpp"
#include "freeusd/usdPhysics/fixedJoint.hpp"
#include "freeusd/usdPhysics/massAPI.hpp"
#include "freeusd/usdPhysics/physicsScene.hpp"
#include "freeusd/usdPhysics/rigidBodyAPI.hpp"
#include "freeusd/usdPhysics/tokens.hpp"
#include "freeusd/usdPhysics/tokens.hpp"
#include "freeusd/usdVol/openVdbAsset.hpp"
#include "freeusd/usdVol/volume.hpp"
#include "freeusd/usdShade/material.hpp"
#include "freeusd/usdShade/previewSurface.hpp"
#include "freeusd/usdUtils/pipeline.hpp"
#include "freeusd/vt/value.hpp"

#ifndef FREEUSD_TEST_FIXTURES_DIR
#error "FREEUSD_TEST_FIXTURES_DIR must be set by CMake for parity_fixtures_test"
#endif

namespace {

std::string fixture(const char* name) {
  return std::string(FREEUSD_TEST_FIXTURES_DIR) + "/" + name;
}

}  // namespace

int main() {
  using freeusd::sdf::Path;
  using freeusd::tf::Token;
  using freeusd::usd::Stage;

  {
    std::string err;
    auto stage = Stage::OpenFromRootFile(fixture("parity_stack_root.usda"),
                                         freeusd::usd::RootLayerSublayersPolicy::DepthFirst, &err);
    assert(stage && err.empty());
    const Path model = Path::FromString("/World/Model");
    freeusd::vt::Value v;
    assert(stage->ReadFieldAtEvaluatedTime(model, Token("stackedOnly"), 15.0, &v));
    double d = 0.0;
    assert(v.GetDouble(&d) && d == 50.0);
    assert(stage->ReadFieldAtEvaluatedTime(model, Token("stackedOnly"), -5.0, &v));
    assert(v.GetDouble(&d) && d == 50.0);
    const auto times = stage->ListComposedFieldSampleTimes(model, Token("stackedOnly"));
    assert(times.size() == 2u);
    assert(times[0] == -5.0);
    assert(times[1] == 15.0);

    auto flat = freeusd::usdUtils::FlattenStageAtTime(*stage, 15.0);
    assert(flat);
    const auto flat_times = flat->ListSampleTimes(model, Token("stackedOnly"));
    assert(flat_times.size() == 2u);
    assert(flat_times[0] == -5.0);
    assert(flat_times[1] == 15.0);
    assert(flat->GetField(model, Token("stackedOnly"), &v));
    assert(v.GetDouble(&d) && d == 50.0);
  }

  {
    std::string err;
    auto stage = Stage::OpenFromRootFile(fixture("parity_namespace.usda"),
                                         freeusd::usd::RootLayerSublayersPolicy::DepthFirst, &err);
    assert(stage && err.empty());
    const Path published = Path::FromString("/Library/Published");
    const Path ref_leaf = Path::FromString("/Library/Published/RefLeaf");
    const Path payload_leaf = Path::FromString("/Library/Published/PayloadLeaf");
    assert(stage->PrimPathInUse(published));
    assert(stage->PrimPathInUse(ref_leaf));
    assert(stage->PrimPathInUse(payload_leaf));
    freeusd::vt::Value v;
    double d = 0.0;
    assert(stage->ReadFieldAtEvaluatedTime(published, Token("refOnly"), 1.0, &v));
    assert(v.GetDouble(&d) && d == 11.0);
    assert(stage->ReadFieldAtEvaluatedTime(ref_leaf, Token("branch"), 1.0, &v));
    assert(v.GetDouble(&d) && d == 22.0);
    assert(stage->ReadFieldAtEvaluatedTime(published, Token("payloadOnly"), 1.0, &v));
    assert(v.GetDouble(&d) && d == 33.0);
    assert(stage->ReadFieldAtEvaluatedTime(payload_leaf, Token("branch"), 1.0, &v));
    assert(v.GetDouble(&d) && d == 44.0);
  }

  {
    std::string err;
    auto stage = Stage::OpenFromRootFile(fixture("parity_variants.usda"),
                                         freeusd::usd::RootLayerSublayersPolicy::DepthFirst, &err);
    assert(stage && err.empty());
    const Path host = Path::FromString("/World/VariantHost");
    const Path child = Path::FromString("/World/VariantHost/VariantChild");
    assert(stage->PrimPathInUse(child));
    freeusd::vt::Value v;
    double d = 0.0;
    assert(stage->ReadFieldAtEvaluatedTime(host, Token("variantValue"), 1.0, &v));
    assert(v.GetDouble(&d) && d == 5.0);
    assert(stage->ReadFieldAtEvaluatedTime(child, Token("branch"), 1.0, &v));
    assert(v.GetDouble(&d) && d == 9.0);
  }

  {
    std::string err;
    freeusd::usd::crate::UsdcCrateStringTable tokens{};
    freeusd::usd::crate::UsdcCrateStringTable strings{};
    freeusd::usd::crate::UsdcCrateFieldsTable fields{};
    freeusd::usd::crate::UsdcCrateSpecsTable specs{};
    freeusd::usd::crate::UsdcCrateFieldSetsTable fieldsets{};
    freeusd::usd::crate::UsdcCrateValuesTable values{};
    freeusd::usd::crate::UsdcCratePathTable paths{};
    assert(freeusd::usd::crate::ReadUsdCrateTokenTableFromPath(fixture("parity_tables.usdc"), tokens, 8, 1024, &err));
    assert(tokens.values.size() == 2u);
    assert(tokens.values[0] == "render");
    assert(tokens.values[1] == "invisible");
    assert(freeusd::usd::crate::ReadUsdCrateStringTableFromPath(fixture("parity_tables.usdc"), strings, 8, 1024, &err));
    assert(strings.values.size() == 2u);
    assert(strings.values[0] == "hello");
    assert(strings.values[1] == "world");
    assert(freeusd::usd::crate::ReadUsdCrateFieldsTableFromPath(fixture("parity_tables.usdc"), fields, 8, 1024, &err));
    assert(fields.entries.size() == 2u);
    assert(fields.entries[0].token_index == 0u && fields.entries[0].value_type_token_index == 1u);
    assert(fields.entries[1].token_index == 1u && fields.entries[1].value_type_token_index == 0u);
    assert(freeusd::usd::crate::ReadUsdCrateFieldSetsTableFromPath(fixture("parity_tables.usdc"), fieldsets, 8, 8, 1024,
                                                                   &err));
    assert(fieldsets.sets.size() == 2u);
    assert(fieldsets.sets[0].field_indices.size() == 2u);
    assert(fieldsets.sets[0].field_indices[0] == 0u);
    assert(fieldsets.sets[0].field_indices[1] == 1u);
    assert(fieldsets.sets[1].field_indices.size() == 1u);
    assert(fieldsets.sets[1].field_indices[0] == 1u);
    freeusd::usd::crate::UsdcCrateTypedValuesTable typed_values{};
    assert(freeusd::usd::crate::ReadUsdCrateTypedValuesTableFromPath(fixture("parity_tables.usdc"), typed_values, 19,
                                                                     1024, &err));
    assert(typed_values.entries.size() == 19u);
    assert(typed_values.entries[0].int32_value == 42);
    assert(typed_values.entries[4].double_value > 3.24 && typed_values.entries[4].double_value < 3.26);
    assert(typed_values.entries[5].int64_value == -9007199254740991LL);
    assert(typed_values.entries[6].string_utf8 == "parity");
    assert(typed_values.entries[7].vec3f_value.data[2] > 2.99f);
    assert(typed_values.entries[8].string_index == 1u);
    assert(typed_values.entries[9].vec3d_value.data[2] > 5.99);
    assert(typed_values.entries[10].int32_array.size() == 3u);
    assert(typed_values.entries[11].float_array.size() == 2u);
    assert(freeusd::usd::crate::ReadUsdCrateValuesTableFromPath(fixture("parity_tables.usdc"), values, 19, 1024, &err));
    assert(values.entries.size() == 19u);
    assert(values.entries[0].bytes.size() == 4u);
    assert(freeusd::usd::crate::ReadUsdCrateTypedValuesTableFromPath(
        fixture("parity_tables_zlib.usdc"), typed_values, 19, 1024, &err));
    assert(typed_values.entries.size() == 19u);
    assert(freeusd::usd::crate::ReadUsdCrateTypedValuesTableFromPath(
        fixture("parity_tables_lz4.usdc"), typed_values, 19, 1024, &err));
    assert(typed_values.entries.size() == 19u);
    assert(typed_values.entries[11].float_array.size() == 2u);
    assert(typed_values.entries[12].double_array.size() == 2u);
    assert(typed_values.entries[13].vec2f_value.data[0] > 0.49f && typed_values.entries[13].vec2f_value.data[1] > 1.24f);
    assert(typed_values.entries[12].double_array[0] > 0.99 && typed_values.entries[12].double_array[1] > 1.99);
    assert(typed_values.entries[14].kind == freeusd::usd::crate::UsdcCrateTypedValueKind::Vec4f);
    assert(typed_values.entries[14].vec4f_value.data[0] > 0.99f && typed_values.entries[14].vec4f_value.data[3] > 3.99f);
    assert(typed_values.entries[15].kind == freeusd::usd::crate::UsdcCrateTypedValueKind::Vec2d);
    assert(typed_values.entries[15].vec2d_value.data[0] > 0.49 && typed_values.entries[15].vec2d_value.data[1] > 1.74);
    assert(typed_values.entries[16].kind == freeusd::usd::crate::UsdcCrateTypedValueKind::Quatf);
    assert(typed_values.entries[16].quatf_value.real > 0.99f && typed_values.entries[16].quatf_value.i > 0.49f);
    assert(typed_values.entries[17].kind == freeusd::usd::crate::UsdcCrateTypedValueKind::Quatd);
    assert(typed_values.entries[17].quatd_value.real > 0.99 && typed_values.entries[17].quatd_value.k > 0.12);
    assert(typed_values.entries[18].kind == freeusd::usd::crate::UsdcCrateTypedValueKind::TokenIndexArray);
    assert(typed_values.entries[18].token_index_array[0] == 0u && typed_values.entries[18].token_index_array[1] == 1u);
    assert(freeusd::usd::crate::ReadUsdCrateSpecsTableFromPath(fixture("parity_tables.usdc"), specs, 8, 1024, &err));
    assert(specs.entries.size() == 2u);
    assert(specs.entries[0].path_index == 0u && specs.entries[0].field_set_index == 0u &&
           specs.entries[0].spec_type == 1u);
    assert(specs.entries[1].path_index == 1u && specs.entries[1].field_set_index == 1u &&
           specs.entries[1].spec_type == 2u);
    assert(freeusd::usd::crate::ReadUsdCratePathTableFromPath(fixture("parity_tables.usdc"), paths, 8, 1024, &err));
    assert(paths.paths.size() == 2u);
    assert(paths.paths[0] == Path::FromString("/World"));
    assert(paths.paths[1] == Path::FromString("/World/Cube"));
  }

  {
    std::string err;
    auto stage = Stage::OpenFromRootFile(fixture("parity_geom_mesh.usda"),
                                         freeusd::usd::RootLayerSublayersPolicy::None, &err);
    assert(stage && err.empty());
    freeusd::usdGeom::Mesh mesh(stage->GetPrimAtPath(Path::FromString("/World/Tri")));
    assert(mesh);
    const auto points = mesh.GetPoints(1.0);
    assert(points.size() == 3u);
    const auto counts = mesh.GetFaceVertexCounts(1.0);
    assert(counts.size() == 1u && counts[0] == 3);
    const auto indices = mesh.GetFaceVertexIndices(1.0);
    assert(indices.size() == 3u);
    const auto colors = mesh.GetDisplayColor(1.0);
    assert(colors.size() == 3u);
    const auto normals = mesh.GetNormals(1.0);
    assert(normals.size() == 3u);
    assert(normals[0].z() == 1.0f);
    const auto st = mesh.GetPrimvarsSt(1.0);
    assert(st.size() == 3u);
    float opacity = 0.0f;
    assert(mesh.GetDisplayOpacity(&opacity, 1.0) && opacity == 0.75f);
    freeusd::gf::Vec3f ext_min{};
    freeusd::gf::Vec3f ext_max{};
    assert(mesh.GetExtent(&ext_min, &ext_max, 1.0));
    assert(ext_max.x() == 1.0f && ext_max.y() == 1.0f);
    std::string scheme;
    assert(mesh.GetSubdivisionScheme(&scheme, 1.0) && scheme == "none");
  }

  {
    std::string err;
    auto stage = Stage::OpenFromRootFile(fixture("parity_shade_texture.usda"),
                                         freeusd::usd::RootLayerSublayersPolicy::None, &err);
    assert(stage && err.empty());
    const freeusd::usdShade::PreviewSurface preview = freeusd::usdShade::PreviewSurface::ReadFromPrim(
        stage, Path::FromString("/World/Looks/Material/PreviewSurface"));
    assert(preview);
    std::string texture_path;
    assert(preview.GetDiffuseTextureAssetPath(&texture_path, 1.0));
    assert(texture_path == "textures/albedo.png");
  }

  {
    std::string err;
    auto stage = Stage::OpenFromRootFile(fixture("parity_shade_pbr_textures.usda"),
                                         freeusd::usd::RootLayerSublayersPolicy::None, &err);
    assert(stage && err.empty());
    const freeusd::usdShade::PreviewSurface preview = freeusd::usdShade::PreviewSurface::ReadFromPrim(
        stage, Path::FromString("/World/Looks/Material/PreviewSurface"));
    assert(preview);
    std::string pbr_path;
    assert(preview.GetRoughnessTextureAssetPath(&pbr_path, 1.0));
    assert(pbr_path == "textures/roughness.png");
  }

  {
    std::string err;
    auto stage = Stage::OpenFromRootFile(fixture("parity_lux_sphere.usda"),
                                         freeusd::usd::RootLayerSublayersPolicy::None, &err);
    assert(stage && err.empty());
    freeusd::usdLux::SphereLight light =
        freeusd::usdLux::SphereLight::ReadFromPrim(stage, Path::FromString("/World/Bulb"));
    assert(light);
    float radius = 0.0f;
    assert(light.GetRadius(&radius, 1.0) && radius == 0.25f);
  }

  {
    std::string err;
    auto stage = Stage::OpenFromRootFile(fixture("parity_lux_rect.usda"), freeusd::usd::RootLayerSublayersPolicy::None,
                                         &err);
    assert(stage && err.empty());
    freeusd::usdLux::RectLight light =
        freeusd::usdLux::RectLight::ReadFromPrim(stage, Path::FromString("/World/Panel"));
    assert(light);
    float width = 0.0f;
    assert(light.GetWidth(&width, 1.0) && width == 2.0f);
  }

  {
    std::string err;
    auto stage = Stage::OpenFromRootFile(fixture("parity_lux_disk.usda"), freeusd::usd::RootLayerSublayersPolicy::None,
                                         &err);
    assert(stage && err.empty());
    freeusd::usdLux::DiskLight light =
        freeusd::usdLux::DiskLight::ReadFromPrim(stage, Path::FromString("/World/Softbox"));
    assert(light);
    float radius = 0.0f;
    assert(light.GetRadius(&radius, 1.0) && radius == 0.75f);
  }

  {
    std::string err;
    auto stage = Stage::OpenFromRootFile(fixture("parity_lux_cylinder.usda"),
                                         freeusd::usd::RootLayerSublayersPolicy::None, &err);
    assert(stage && err.empty());
    freeusd::usdLux::CylinderLight light =
        freeusd::usdLux::CylinderLight::ReadFromPrim(stage, Path::FromString("/World/Tube"));
    assert(light);
    float length = 0.0f;
    assert(light.GetLength(&length, 1.0) && length == 2.5f);
  }

  {
    std::string err;
    auto stage = Stage::OpenFromRootFile(fixture("parity_lux_dome.usda"), freeusd::usd::RootLayerSublayersPolicy::None,
                                         &err);
    assert(stage && err.empty());
    freeusd::usdLux::DomeLight light =
        freeusd::usdLux::DomeLight::ReadFromPrim(stage, Path::FromString("/World/Sky"));
    assert(light);
    std::string texture_path;
    assert(light.GetTextureFileAssetPath(&texture_path, 1.0));
    assert(texture_path == "textures/sky.hdr");
  }

  {
    std::string err;
    auto stage = Stage::OpenFromRootFile(fixture("parity_embedded_scene.usdc"),
                                         freeusd::usd::RootLayerSublayersPolicy::None, &err);
    assert(stage && err.empty());
    assert(stage->HasDefaultPrim());
    assert(stage->GetDefaultPrimName() == "World");
    assert(stage->PrimPathInUse(Path::FromString("/World/Cube")));
  }

  {
    std::string err;
    auto stage = Stage::OpenFromRootFile(fixture("parity_embedded_scene_zlib.usdc"),
                                         freeusd::usd::RootLayerSublayersPolicy::None, &err);
    assert(stage && err.empty());
    assert(stage->HasDefaultPrim());
    assert(stage->GetDefaultPrimName() == "World");
    const Path cube = Path::FromString("/World/Cube");
    assert(stage->PrimPathInUse(cube));
    freeusd::vt::Value v;
    assert(stage->ReadFieldAtEvaluatedTime(cube, Token("size"), 1.0, &v));
    double d = 0.0;
    assert(v.GetDouble(&d) && d == 2.0);
  }


  {
    std::string err;
    auto stage = Stage::OpenFromRootFile(fixture("parity_embedded_scene_lz4.usdc"),
                                         freeusd::usd::RootLayerSublayersPolicy::None, &err);
    assert(stage && err.empty());
    assert(stage->HasDefaultPrim());
    const Path cube = Path::FromString("/World/Cube");
    assert(stage->PrimPathInUse(cube));
    freeusd::vt::Value v;
    assert(stage->ReadFieldAtEvaluatedTime(cube, Token("size"), 1.0, &v));
    double d = 0.0;
    assert(v.GetDouble(&d) && d == 2.0);
  }

  {
    std::string err;
    auto stage = Stage::OpenFromRootFile(fixture("parity_custom_data_inherit.usda"),
                                         freeusd::usd::RootLayerSublayersPolicy::DepthFirst, &err);
    assert(stage && err.empty());
    const Path host = Path::FromString("/World/Host");
    const auto host_prim = stage->GetPrimAtPath(host);
    assert(host_prim.IsValid());
    freeusd::vt::Value v;
    std::string role;
    assert(stage->GetComposedPrimCustomData(host, "role", &v));
    assert(v.GetString(&role) && role == "base");
    assert(stage->PrimCustomDataKeyInAnyLayer(host, "role"));
    assert(host_prim.HasCustomDataKey("role"));
    std::int32_t priority = 0;
    assert(stage->GetComposedPrimCustomData(host, "priority", &v));
    assert(v.GetInt32(&priority) && priority == 9);
    const auto keys = stage->ListComposedPrimCustomDataKeys(host);
    assert(keys.size() == 2u);
    bool has_role = false;
    bool has_priority = false;
    for (const std::string& k : keys) {
      if (k == "role") {
        has_role = true;
      }
      if (k == "priority") {
        has_priority = true;
      }
    }
    assert(has_role && has_priority);
    assert(stage->ResolvePrimSpecifierKind(host) == freeusd::sdf::Layer::PrimSpecifierKind::Class);
    assert(host_prim.IsAbstract());
  }

  {
    std::string err;
    auto stage = Stage::OpenFromRootFile(fixture("parity_custom_data_refs.usda"),
                                         freeusd::usd::RootLayerSublayersPolicy::DepthFirst, &err);
    assert(stage && err.empty());
    const Path ref_host = Path::FromString("/World/RefHost");
    const Path payload_host = Path::FromString("/World/PayloadHost");
    freeusd::vt::Value v;
    std::string role;
    assert(stage->GetComposedPrimCustomData(ref_host, "role", &v));
    assert(v.GetString(&role) && role == "from_ref");
    std::int32_t priority = 0;
    assert(stage->GetComposedPrimCustomData(ref_host, "priority", &v));
    assert(v.GetInt32(&priority) && priority == 9);
    assert(stage->GetComposedPrimCustomData(payload_host, "role", &v));
    assert(v.GetString(&role) && role == "from_payload");
    assert(stage->GetComposedPrimCustomData(payload_host, "priority", &v));
    assert(v.GetInt32(&priority) && priority == 3);
    const auto ref_keys = stage->ListComposedPrimCustomDataKeys(ref_host);
    assert(ref_keys.size() == 2u);
    const auto payload_keys = stage->ListComposedPrimCustomDataKeys(payload_host);
    assert(payload_keys.size() == 2u);
  }

  {
    std::string err;
    auto stage = Stage::OpenFromRootFile(fixture("parity_custom_data_specializes.usda"),
                                         freeusd::usd::RootLayerSublayersPolicy::DepthFirst, &err);
    assert(stage && err.empty());
    const Path host = Path::FromString("/World/Host");
    freeusd::vt::Value v;
    std::string role;
    assert(stage->GetComposedPrimCustomData(host, "role", &v));
    assert(v.GetString(&role) && role == "from_spec");
    std::int32_t priority = 0;
    assert(stage->GetComposedPrimCustomData(host, "priority", &v));
    assert(v.GetInt32(&priority) && priority == 9);
    const auto keys = stage->ListComposedPrimCustomDataKeys(host);
    assert(keys.size() == 2u);
  }

  {
    std::string err;
    auto stage = Stage::OpenFromRootFile(fixture("parity_variant_selection_refs.usda"),
                                         freeusd::usd::RootLayerSublayersPolicy::DepthFirst, &err);
    assert(stage && err.empty());
    const Path ref_host = Path::FromString("/World/RefHost");
    std::string variant;
    assert(stage->GetComposedPrimVariantSelection(ref_host, "modelVariant", &variant));
    assert(variant == "B");
    const auto sets = stage->ListComposedPrimVariantSelectionSets(ref_host);
    assert(sets.size() == 1u && sets[0] == "modelVariant");
    freeusd::vt::Value v;
    assert(stage->ReadFieldAtEvaluatedTime(ref_host, Token("variantValue"), 1.0, &v));
    double d = 0.0;
    assert(v.GetDouble(&d) && d == 9.0);
  }


  {
    std::string err;
    auto stage = Stage::OpenFromRootFile(fixture("parity_variant_sets_refs.usda"),
                                         freeusd::usd::RootLayerSublayersPolicy::DepthFirst, &err);
    assert(stage && err.empty());
    const Path ref_host = Path::FromString("/World/RefHost");
    const auto set_names = stage->ListComposedPrimVariantSetNames(ref_host);
    assert(set_names.size() == 1u && set_names[0] == "modelVariant");
    assert(stage->PrimVariantSetDeclaredInAnyLayer(ref_host, "modelVariant"));
    const auto variant_names = stage->GetComposedPrimVariantNames(ref_host, "modelVariant");
    assert(variant_names.size() == 2u);
    assert(variant_names[0] == "A" && variant_names[1] == "B");
    std::string variant;
    assert(stage->GetComposedPrimVariantSelection(ref_host, "modelVariant", &variant));
    assert(variant == "B");
    freeusd::vt::Value v;
    assert(stage->ReadFieldAtEvaluatedTime(ref_host, Token("variantValue"), 1.0, &v));
    double d = 0.0;
    assert(v.GetDouble(&d) && d == 9.0);
  }
  {
    std::string err;
    auto stage = Stage::OpenFromRootFile(fixture("parity_specializes.usda"),
                                         freeusd::usd::RootLayerSublayersPolicy::DepthFirst, &err);
    assert(stage && err.empty());
    const Path host = Path::FromString("/World/Host");
    const Path base = Path::FromString("/World/BaseSpec");
    const auto host_prim = stage->GetPrimAtPath(host);
    assert(host_prim.IsValid());
    assert(host_prim.HasSpecializes());
    const auto specs = stage->ReadPrimSpecializes(host);
    assert(specs.size() == 1u);
    assert(specs[0] == base);
    freeusd::vt::Value v;
    double d = 0.0;
    assert(stage->ReadFieldAtEvaluatedTime(host, Token("sharedStrength"), 1.0, &v));
    assert(v.GetDouble(&d) && d == 10.0);
    assert(stage->ReadFieldAtEvaluatedTime(host, Token("fromSpec"), 1.0, &v));
    assert(v.GetDouble(&d) && d == 99.0);
    assert(stage->ReadFieldAtEvaluatedTime(host, Token("hostOnly"), 1.0, &v));
    assert(v.GetDouble(&d) && d == 3.0);
    bool has_shared = false;
    for (const std::string& name : stage->ListComposedFieldNames(host)) {
      if (name == "sharedStrength") {
        has_shared = true;
      }
    }
    assert(has_shared);
    assert(!stage->ReadFieldAtEvaluatedTime(base, Token("hostOnly"), 1.0, &v));
  }

  {
    std::string err;
    auto stage = Stage::OpenFromRootFile(fixture("parity_kind_active_refs.usda"),
                                         freeusd::usd::RootLayerSublayersPolicy::DepthFirst, &err);
    assert(stage && err.empty());
    const Path ref_host = Path::FromString("/World/RefHost");
    const Path payload_host = Path::FromString("/World/PayloadHost");
    const Path inherit_host = Path::FromString("/World/InheritHost");
    assert(stage->PrimPathInUse(ref_host));
    assert(stage->PrimPathInUse(payload_host));
    assert(stage->ResolveHasPrimKind(ref_host));
    assert(stage->ResolvePrimKind(ref_host).GetText() == std::string("component"));
    assert(stage->GetPrimAtPath(ref_host).GetPrimKind().GetText() == std::string("component"));
    assert(stage->ResolveHasPrimActiveOpinion(ref_host));
    assert(!stage->ResolvePrimActive(ref_host));
    assert(!stage->GetPrimAtPath(ref_host).IsActive());
    assert(stage->ResolveHasPrimKind(payload_host));
    assert(stage->ResolvePrimKind(payload_host).GetText() == std::string("group"));
    assert(stage->ResolveHasPrimKind(inherit_host));
    assert(stage->ResolvePrimKind(inherit_host).GetText() == std::string("assembly"));
    assert(stage->ResolvePrimActive(inherit_host));
    assert(!stage->ResolveHasPrimActiveOpinion(inherit_host));
  }

  {
    std::string err;
    auto stage = Stage::OpenFromRootFile(fixture("parity_imageable.usda"),
                                         freeusd::usd::RootLayerSublayersPolicy::DepthFirst, &err);
    assert(stage && err.empty());
    const auto cube_prim = stage->GetPrimAtPath(Path::FromString("/World/Cube"));
    const freeusd::usdGeom::Imageable cube(cube_prim);
    assert(cube.ComputePurpose(1.0) == "render");
    assert(!cube.ComputeVisibility(1.0));
    const freeusd::usdGeom::Boundable boundable(cube_prim);
    const freeusd::gf::BBox3d world = boundable.ComputeWorldBound(1.0);
    assert(!world.IsEmpty());
    assert(world.min.x() == 0.0 && world.min.y() == 1.0 && world.min.z() == 2.0);
    assert(world.max.x() == 2.0 && world.max.y() == 3.0 && world.max.z() == 4.0);
  }

  {
    std::string err;
    auto stage = Stage::OpenFromRootFile(fixture("parity_skel_gltf.usda"),
                                         freeusd::usd::RootLayerSublayersPolicy::DepthFirst, &err);
    assert(stage && err.empty());
    const freeusd::usdSkel::Skeleton skel(stage->GetPrimAtPath(Path::FromString("/World/Character/Skeleton")));
    assert(skel);
    const std::vector<std::string> joints = skel.GetJointNames();
    assert(joints.size() == 2u && joints[0] == "Root" && joints[1] == "Root/Hip");
    const freeusd::usdSkel::SkelAnimation anim(stage->GetPrimAtPath(Path::FromString("/World/Character/Anim")));
    assert(anim);
    std::vector<freeusd::gf::Vec3f> translations{};
    assert(anim.GetTranslations(&translations, 1.0));
    assert(translations.size() == 2u);
    assert(translations[1].y() == 2.0f);
  }

  {
    std::string err;
    auto stage = Stage::OpenFromRootFile(fixture("parity_skel_binding.usda"),
                                         freeusd::usd::RootLayerSublayersPolicy::DepthFirst, &err);
    assert(stage && err.empty());
    const freeusd::usdSkel::SkelRoot root(
        freeusd::usdSkel::SkelRoot::ReadFromPrim(stage, Path::FromString("/World/SkelCharacter")));
    assert(root);
    const freeusd::usd::Prim body = stage->GetPrimAtPath(Path::FromString("/World/SkelCharacter/Body"));
    const freeusd::usdSkel::SkelBinding binding = freeusd::usdSkel::SkelBinding::ReadFromGeomPrim(body);
    assert(binding);
    std::vector<int> indices{};
    std::vector<float> weights{};
    assert(freeusd::usdSkel::SkelBinding::ReadJointIndices(body, &indices));
    assert(freeusd::usdSkel::SkelBinding::ReadJointWeights(body, &weights));
    assert(freeusd::usdSkel::SkelBinding::ValidateInfluenceCounts(indices, weights));
    const auto bound_paths = root.FindBoundGeomPaths();
    assert(bound_paths.size() == 1u);
    assert(bound_paths[0] == Path::FromString("/World/SkelCharacter/Body"));
  }

  {
    std::string err;
    auto stage = Stage::OpenFromRootFile(fixture("parity_skel_blend_shapes.usda"),
                                         freeusd::usd::RootLayerSublayersPolicy::DepthFirst, &err);
    assert(stage && err.empty());
    const freeusd::usd::Prim face = stage->GetPrimAtPath(Path::FromString("/World/Face"));
    const freeusd::usdSkel::MorphTargets morphs = freeusd::usdSkel::MorphTargets::ReadFromGeomPrim(face);
    assert(morphs);
    assert(morphs.GetBlendShapeTokens().size() == 2u);
    std::vector<freeusd::gf::Vec3f> morphed{};
    assert(morphs.EvaluatePoints(&morphed, 1.0));
    assert(morphed.size() == 2u);
    assert(morphed[0].y() == 1.0f);
    assert(morphed[0].z() == 0.5f);
  }

  {
    std::string err;
    auto stage = Stage::OpenFromRootFile(fixture("parity_shade_preview.usda"),
                                         freeusd::usd::RootLayerSublayersPolicy::DepthFirst, &err);
    assert(stage && err.empty());
    const freeusd::usdShade::Material mat(
        freeusd::usdShade::Material::ReadFromPrim(stage, Path::FromString("/World/Looks/Material")));
    assert(mat);
    const Path shader_path = mat.GetSurfaceShaderPath();
    assert(shader_path == Path::FromString("/World/Looks/Material/PreviewSurface"));
    const freeusd::usdShade::PreviewSurface preview = freeusd::usdShade::PreviewSurface::ReadFromPrim(stage, shader_path);
    assert(preview && preview.IsPreviewSurface());
    freeusd::gf::Vec3f diffuse{};
    assert(preview.GetDiffuseColor(&diffuse, 1.0));
    assert(diffuse.x() == 0.8f && diffuse.y() == 0.2f && diffuse.z() == 0.1f);
  }

  {
    std::string err;
    auto stage = Stage::OpenFromRootFile(fixture("parity_physics_scene.usda"),
                                         freeusd::usd::RootLayerSublayersPolicy::DepthFirst, &err);
    assert(stage && err.empty());
    const freeusd::usdPhysics::PhysicsScene scene =
        freeusd::usdPhysics::PhysicsScene::ReadFromPrim(stage, Path::FromString("/World/Physics"));
    assert(scene && scene.IsPhysicsScene());
    freeusd::gf::Vec3f gravity_dir{};
    assert(scene.GetGravityDirection(&gravity_dir, 1.0));
    assert(gravity_dir.z() == -1.0f);
    float gravity_mag = 0.0f;
    assert(scene.GetGravityMagnitude(&gravity_mag, 1.0) && gravity_mag == 981.0f);
  }

  {
    std::string err;
    auto stage = Stage::OpenFromRootFile(fixture("parity_physics_rigid_body.usda"),
                                         freeusd::usd::RootLayerSublayersPolicy::DepthFirst, &err);
    assert(stage && err.empty());
    const freeusd::usdPhysics::RigidBodyAPI body =
        freeusd::usdPhysics::RigidBodyAPI::ReadFromPrim(stage, Path::FromString("/World/Body"));
    assert(body && body.IsRigidBodyAPI());
    float mass = 0.0f;
    assert(body.GetMass(&mass, 1.0) && mass == 2.5f);
  }

  {
    std::string err;
    auto stage = Stage::OpenFromRootFile(fixture("parity_physics_rigid_body_kinematic.usda"),
                                         freeusd::usd::RootLayerSublayersPolicy::DepthFirst, &err);
    assert(stage && err.empty());
    const freeusd::usdPhysics::RigidBodyAPI body =
        freeusd::usdPhysics::RigidBodyAPI::ReadFromPrim(stage, Path::FromString("/World/Body"));
    assert(body && body.IsRigidBodyAPI());
    bool kinematic_enabled = false;
    assert(body.GetKinematicEnabled(&kinematic_enabled, 1.0) && kinematic_enabled);
  }

  {
    std::string err;
    auto stage = Stage::OpenFromRootFile(fixture("parity_physics_mass.usda"),
                                         freeusd::usd::RootLayerSublayersPolicy::DepthFirst, &err);
    assert(stage && err.empty());
    const freeusd::usdPhysics::MassAPI prop =
        freeusd::usdPhysics::MassAPI::ReadFromPrim(stage, Path::FromString("/World/Prop"));
    assert(prop && prop.IsMassAPI());
    float density = 0.0f;
    assert(prop.GetDensity(&density, 1.0) && density == 2.0f);
    freeusd::gf::Vec3f com{};
    assert(prop.GetCenterOfMass(&com, 1.0));
    assert(com.y() == 0.5f);
  }

  {
    std::string err;
    auto stage = Stage::OpenFromRootFile(fixture("parity_physics_fixed_joint.usda"),
                                         freeusd::usd::RootLayerSublayersPolicy::DepthFirst, &err);
    assert(stage && err.empty());
    const freeusd::usdPhysics::FixedJoint joint =
        freeusd::usdPhysics::FixedJoint::ReadFromPrim(stage, Path::FromString("/World/Anchor"));
    assert(joint && joint.IsFixedJoint());
    freeusd::sdf::Path body0;
    freeusd::sdf::Path body1;
    assert(joint.GetBody0(&body0) && joint.GetBody1(&body1));
    assert(body0 == Path::FromString("/World/BodyA"));
    assert(body1 == Path::FromString("/World/BodyB"));
    bool enabled = false;
    assert(joint.GetJointEnabled(&enabled, 1.0) && enabled);
  }

  {
    std::string err;
    auto stage = Stage::OpenFromRootFile(fixture("parity_physics_rigid_body_refs.usda"),
                                         freeusd::usd::RootLayerSublayersPolicy::DepthFirst, &err);
    assert(stage && err.empty());
    const freeusd::usdPhysics::RigidBodyAPI body =
        freeusd::usdPhysics::RigidBodyAPI::ReadFromPrim(stage, Path::FromString("/World/RefHost"));
    assert(body && body.IsRigidBodyAPI());
    float mass = 0.0f;
    assert(body.GetMass(&mass, 1.0) && mass == 7.0f);
  }

  {
    std::string err;
    auto stage = Stage::OpenFromRootFile(fixture("parity_physics_rigid_body_inherit.usda"),
                                         freeusd::usd::RootLayerSublayersPolicy::DepthFirst, &err);
    assert(stage && err.empty());
    const freeusd::usdPhysics::RigidBodyAPI body =
        freeusd::usdPhysics::RigidBodyAPI::ReadFromPrim(stage, Path::FromString("/World/Body"));
    assert(body && body.IsRigidBodyAPI());
    freeusd::vt::Value api_schemas;
    assert(stage->ReadFieldAtEvaluatedTime(Path::FromString("/World/Body"), freeusd::tf::Token{"apiSchemas"}, 1.0,
                                           &api_schemas));
    std::vector<freeusd::tf::Token> schemas;
    assert(api_schemas.GetTokenArray(&schemas) && schemas.size() == 1u);
    assert(schemas[0] == freeusd::usdPhysics::tokens::PhysicsRigidBodyAPI());
    float mass = 0.0f;
    assert(body.GetMass(&mass, 1.0) && mass == 4.0f);
  }

  {
    std::string err;
    auto stage = Stage::OpenFromRootFile(fixture("parity_physics_collision.usda"),
                                         freeusd::usd::RootLayerSublayersPolicy::DepthFirst, &err);
    assert(stage && err.empty());
    const freeusd::usdPhysics::CollisionAPI collider =
        freeusd::usdPhysics::CollisionAPI::ReadFromPrim(stage, Path::FromString("/World/Collider"));
    assert(collider && collider.IsCollisionAPI());
    bool enabled = true;
    assert(collider.GetCollisionEnabled(&enabled, 1.0) && !enabled);
  }

  {
    std::string err;
    auto stage = Stage::OpenFromRootFile(fixture("parity_physics_collision_inherit.usda"),
                                         freeusd::usd::RootLayerSublayersPolicy::DepthFirst, &err);
    assert(stage && err.empty());
    const freeusd::usdPhysics::CollisionAPI collider =
        freeusd::usdPhysics::CollisionAPI::ReadFromPrim(stage, Path::FromString("/World/Collider"));
    assert(collider && collider.IsCollisionAPI());
    bool enabled = true;
    assert(collider.GetCollisionEnabled(&enabled, 1.0) && !enabled);
    freeusd::vt::Value api_schemas;
    assert(stage->ReadFieldAtEvaluatedTime(Path::FromString("/World/Collider"), freeusd::tf::Token{"apiSchemas"}, 1.0,
                                           &api_schemas));
    std::vector<freeusd::tf::Token> schemas;
    assert(api_schemas.GetTokenArray(&schemas) && schemas.size() == 1u);
    assert(schemas[0] == freeusd::usdPhysics::tokens::PhysicsCollisionAPI());
  }

  {
    std::string err;
    auto stage = Stage::OpenFromRootFile(fixture("parity_vol_openvdb.usda"),
                                         freeusd::usd::RootLayerSublayersPolicy::DepthFirst, &err);
    assert(stage && err.empty());
    const freeusd::usdVol::OpenVDBAsset field =
        freeusd::usdVol::OpenVDBAsset::ReadFromPrim(stage, Path::FromString("/World/Smoke"));
    assert(field && field.IsOpenVDBAsset());
    std::string file_path;
    assert(field.GetFilePath(&file_path, 1.0));
    assert(file_path == "volumes/smoke.vdb");
    std::string field_name;
    assert(field.GetFieldName(&field_name, 1.0));
    assert(field_name == "density");
  }

  {
    std::string err;
    auto stage = Stage::OpenFromRootFile(fixture("parity_vol_volume.usda"),
                                         freeusd::usd::RootLayerSublayersPolicy::DepthFirst, &err);
    assert(stage && err.empty());
    const freeusd::usdVol::Volume cloud =
        freeusd::usdVol::Volume::ReadFromPrim(stage, Path::FromString("/World/Cloud"));
    assert(cloud && cloud.IsVolume());
    const std::vector<Path> field_targets = cloud.GetFieldRelationshipTargets();
    assert(field_targets.size() == 1u);
    assert(field_targets[0] == Path::FromString("/World/Cloud/Smoke"));
    const std::vector<freeusd::usdVol::OpenVDBAsset> fields = cloud.GetOpenVDBFieldAssets();
    assert(fields.size() == 1u && fields[0].IsOpenVDBAsset());
    std::string file_path;
    assert(fields[0].GetFilePath(&file_path, 1.0));
    assert(file_path == "volumes/cloud/smoke.vdb");
    std::string field_name;
    assert(fields[0].GetFieldName(&field_name, 1.0));
    assert(field_name == "density");
  }

  return 0;
}
