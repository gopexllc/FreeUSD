#include <cassert>
#include <string>

#include "freeusd/sdf/path.hpp"
#include "freeusd/usd/stage.hpp"
#include "freeusd/usdVol/volume.hpp"

#ifndef FREEUSD_TEST_FIXTURES_DIR
#error "FREEUSD_TEST_FIXTURES_DIR must be set by CMake for usd_vol_volume_test"
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
  using freeusd::usdVol::Volume;

  std::string err;
  auto stage = Stage::OpenFromRootFile(fixture("parity_vol_volume.usda"),
                                       freeusd::usd::RootLayerSublayersPolicy::DepthFirst, &err);
  assert(stage && err.empty());

  const Volume cloud = Volume::ReadFromPrim(stage, Path::FromString("/World/Cloud"));
  assert(cloud);
  assert(cloud.IsVolume());

  const std::vector<Path> field_targets = cloud.GetFieldRelationshipTargets();
  assert(field_targets.size() == 1u);
  assert(field_targets[0] == Path::FromString("/World/Cloud/Smoke"));

  const std::vector<OpenVDBAsset> fields = cloud.GetOpenVDBFieldAssets();
  assert(fields.size() == 1u);
  assert(fields[0].IsOpenVDBAsset());
  assert(fields[0].prim.GetPath() == Path::FromString("/World/Cloud/Smoke"));

  std::string file_path;
  assert(fields[0].GetFilePath(&file_path, 1.0));
  assert(file_path == "volumes/cloud/smoke.vdb");

  std::string field_name;
  assert(fields[0].GetFieldName(&field_name, 1.0));
  assert(field_name == "density");

  return 0;
}
