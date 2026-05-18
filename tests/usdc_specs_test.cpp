#include <cassert>
#include <string>

#include "freeusd/usd/crateFile.hpp"

#ifndef FREEUSD_TEST_FIXTURES_DIR
#error "FREEUSD_TEST_FIXTURES_DIR must be set by CMake for usdc_specs_test"
#endif

namespace {

std::string fixture(const char* name) {
  return std::string(FREEUSD_TEST_FIXTURES_DIR) + "/" + name;
}

}  // namespace

int main() {
  using freeusd::usd::crate::UsdcCrateSpecsTable;

  {
    std::string err;
    UsdcCrateSpecsTable specs{};
    assert(freeusd::usd::crate::ReadUsdCrateSpecsTableFromPath(fixture("parity_tables.usdc"), specs, 8, 1024, &err));
    assert(err.empty());
    assert(specs.entries.size() == 2u);
    assert(specs.entries[0].path_index == 0u);
    assert(specs.entries[0].field_set_index == 0u);
    assert(specs.entries[0].spec_type == 1u);
    assert(specs.entries[1].path_index == 1u);
    assert(specs.entries[1].field_set_index == 1u);
    assert(specs.entries[1].spec_type == 2u);
  }

  {
    std::string err;
    UsdcCrateSpecsTable specs{};
    assert(!freeusd::usd::crate::ReadUsdCrateSpecsTableFromPath(fixture("parity_tables.usdc"), specs, 1, 1024, &err));
    assert(specs.entries.empty());
  }

  {
    std::string err;
    UsdcCrateSpecsTable specs{};
    assert(!freeusd::usd::crate::ReadUsdCrateSpecsTableFromPath(fixture("parity_tables.usdc") + ".missing", specs, 8,
                                                                 1024, &err));
    assert(specs.entries.empty());
  }

  return 0;
}
