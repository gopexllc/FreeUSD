#include <cassert>
#include <string>

#include "freeusd/sdf/path.hpp"
#include "freeusd/usd/stage.hpp"
#include "freeusd/usdVol/openVdbAsset.hpp"

#ifndef FREEUSD_TEST_FIXTURES_DIR
#error "FREEUSD_TEST_FIXTURES_DIR must be set by CMake for usd_vol_openvdb_test"
#endif

namespace {

std::string fixture(const char* name) {
  return std::string(FREEUSD_TEST_FIXTURES_DIR) + "/" + name;
}

}  // namespace

int main() {
  using freeusd::sdf::Path;
  using freeusd::usd::Stage;
  using freeusd::usdVol::OpenVDBAsset;

  std::string err;
  auto stage = Stage::OpenFromRootFile(fixture("parity_vol_openvdb.usda"),
                                       freeusd::usd::RootLayerSublayersPolicy::DepthFirst, &err);
  assert(stage && err.empty());

  const OpenVDBAsset field = OpenVDBAsset::ReadFromPrim(stage, Path::FromString("/World/Smoke"));
  assert(field);
  assert(field.IsOpenVDBAsset());

  std::string file_path;
  assert(field.GetFilePath(&file_path, 1.0));
  assert(file_path == "volumes/smoke.vdb");

  std::string field_name;
  assert(field.GetFieldName(&field_name, 1.0));
  assert(field_name == "density");

  return 0;
}
