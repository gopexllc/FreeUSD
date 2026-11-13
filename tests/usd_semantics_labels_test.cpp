#include <algorithm>
#include <cassert>
#include <string>

#include "freeusd/sdf/path.hpp"
#include "freeusd/usd/stage.hpp"
#include "freeusd/usdSemantics/labelsAPI.hpp"

#ifndef FREEUSD_TEST_FIXTURES_DIR
#error "FREEUSD_TEST_FIXTURES_DIR must be set by CMake for usd_semantics_labels_test"
#endif

namespace {

std::string fixture(const char* name) {
  return std::string(FREEUSD_TEST_FIXTURES_DIR) + "/" + name;
}

}  // namespace

int main() {
  using freeusd::sdf::Path;
  using freeusd::usd::RootLayerSublayersPolicy;
  using freeusd::usd::Stage;
  using freeusd::usdSemantics::SemanticLabelsAPI;

  std::string err;
  auto stage =
      Stage::OpenFromRootFile(fixture("parity_semantics_labels.usda"), RootLayerSublayersPolicy::DepthFirst, &err);
  assert(stage && err.empty());

  const auto cup = SemanticLabelsAPI::ReadFromPrim(stage, Path::FromString("/World/Kitchen/CupBlue"));
  assert(cup);
  assert(cup.HasLabels("somaHome"));
  assert(cup.HasLabels("engine"));
  assert(!cup.HasLabels("missing"));
  assert(cup.ListLabelSetNames() == std::vector<std::string>({"engine", "somaHome"}));
  assert(cup.GetLabels("somaHome") == std::vector<std::string>({"Crockery", "DesignedContainer"}));
  assert(cup.GetLabels("engine") == std::vector<std::string>({"pickup", "container"}));
  assert(cup.GetLabels("missing").empty());

  const auto stove = SemanticLabelsAPI::ReadFromPrim(stage, Path::FromString("/World/Kitchen/Stove"));
  assert(stove.GetLabels("somaHome") == std::vector<std::string>({"Appliance"}));
  assert(stove.ListLabelSetNames() == std::vector<std::string>({"somaHome"}));

  return 0;
}
