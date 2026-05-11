#include <cassert>
#include <string>

#include "freeusd/sdf/path.hpp"
#include "freeusd/tf/token.hpp"
#include "freeusd/usd/crateFile.hpp"
#include "freeusd/usd/stage.hpp"
#include "freeusd/usdGeom/boundable.hpp"
#include "freeusd/usdGeom/imageable.hpp"
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
    freeusd::usd::crate::UsdcCratePathTable paths{};
    assert(freeusd::usd::crate::ReadUsdCrateTokenTableFromPath(fixture("parity_tables.usdc"), tokens, 8, 1024, &err));
    assert(tokens.values.size() == 2u);
    assert(tokens.values[0] == "render");
    assert(tokens.values[1] == "invisible");
    assert(freeusd::usd::crate::ReadUsdCrateStringTableFromPath(fixture("parity_tables.usdc"), strings, 8, 1024, &err));
    assert(strings.values.size() == 2u);
    assert(strings.values[0] == "hello");
    assert(strings.values[1] == "world");
    assert(freeusd::usd::crate::ReadUsdCratePathTableFromPath(fixture("parity_tables.usdc"), paths, 8, 1024, &err));
    assert(paths.paths.size() == 2u);
    assert(paths.paths[0] == Path::FromString("/World"));
    assert(paths.paths[1] == Path::FromString("/World/Cube"));
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

  return 0;
}
