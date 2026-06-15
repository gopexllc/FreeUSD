#include <algorithm>
#include <cassert>
#include <cmath>
#include <string>

#include "freeusd/sdf/path.hpp"
#include "freeusd/usd/stage.hpp"
#include "freeusd/usdUtils/engineScene.hpp"

#ifndef FREEUSD_TEST_FIXTURES_DIR
#error "FREEUSD_TEST_FIXTURES_DIR must be set by CMake for usdutils_spatial_grounding_test"
#endif

namespace {

std::string fixture(const char* name) {
  return std::string(FREEUSD_TEST_FIXTURES_DIR) + "/" + name;
}

bool near(double a, double b) {
  return std::fabs(a - b) < 1e-9;
}

const freeusd::usdUtils::EngineSpatialGroundingRecord* find_record(
    const std::vector<freeusd::usdUtils::EngineSpatialGroundingRecord>& records, const freeusd::sdf::Path& path) {
  for (const auto& record : records) {
    if (record.path == path) {
      return &record;
    }
  }
  return nullptr;
}

bool has_sibling(const freeusd::usdUtils::EngineSpatialGroundingRecord& record, const std::string& name) {
  return std::find(record.sibling_names.begin(), record.sibling_names.end(), name) != record.sibling_names.end();
}

}  // namespace

int main() {
  using freeusd::sdf::Path;
  using freeusd::usd::RootLayerSublayersPolicy;
  using freeusd::usd::Stage;
  using freeusd::usdUtils::BuildEngineSpatialGroundingContext;

  std::string err;
  auto stage =
      Stage::OpenFromRootFile(fixture("parity_spatial_grounding.usda"), RootLayerSublayersPolicy::DepthFirst, &err);
  assert(stage && err.empty());

  const auto records = BuildEngineSpatialGroundingContext(*stage, 1.0);
  const auto* cup = find_record(records, Path::FromString("/World/Kitchen/CupBlue"));
  const auto* kitchen = find_record(records, Path::FromString("/World/Kitchen"));
  assert(cup != nullptr);
  assert(kitchen != nullptr);

  assert(cup->name == "CupBlue");
  assert(cup->parent_path == Path::FromString("/World/Kitchen"));
  assert(cup->sibling_names.size() == 2u);
  assert(has_sibling(*cup, "PlateGreen"));
  assert(has_sibling(*cup, "Stove"));
  assert(cup->semantic_label_sets.size() == 2u);
  assert(cup->semantic_label_sets[0].name == "engine");
  assert(cup->semantic_label_sets[0].labels == std::vector<std::string>({"pickup", "container"}));
  assert(cup->semantic_label_sets[1].name == "somaHome");
  assert(cup->semantic_label_sets[1].labels == std::vector<std::string>({"Crockery", "DesignedContainer"}));

  assert(near(cup->world_position.x(), 6.0));
  assert(near(cup->world_position.y(), 2.0));
  assert(near(cup->world_position.z(), 3.0));
  assert(cup->has_world_bound);
  assert(near(cup->world_bound_dimensions.x(), 0.5));
  assert(near(cup->world_bound_dimensions.y(), 1.5));
  assert(near(cup->world_bound_dimensions.z(), 0.25));
  assert(cup->mass_kg.has_value());
  assert(near(*cup->mass_kg, 0.35));

  assert(kitchen->name == "Kitchen");
  assert(kitchen->parent_path == Path::FromString("/World"));
  assert(kitchen->sibling_names.empty());
  assert(kitchen->semantic_label_sets.empty());
  assert(near(kitchen->world_position.x(), 10.0));
  assert(near(kitchen->world_position.y(), 0.0));
  assert(near(kitchen->world_position.z(), 0.0));
  assert(!kitchen->has_world_bound);
  assert(!kitchen->mass_kg.has_value());

  return 0;
}
