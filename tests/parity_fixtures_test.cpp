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

  return 0;
}
