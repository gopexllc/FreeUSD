#include <cassert>
#include <string>

#include "freeusd/usd/crateFile.hpp"

#ifndef FREEUSD_TEST_FIXTURES_DIR
#error "FREEUSD_TEST_FIXTURES_DIR must be set by CMake for usdc_fieldsets_test"
#endif

namespace {

std::string fixture(const char* name) {
  return std::string(FREEUSD_TEST_FIXTURES_DIR) + "/" + name;
}

}  // namespace

int main() {
  using freeusd::usd::crate::UsdcCrateFieldSetsTable;

  {
    std::string err;
    UsdcCrateFieldSetsTable sets{};
    assert(freeusd::usd::crate::ReadUsdCrateFieldSetsTableFromPath(fixture("parity_tables.usdc"), sets, 8, 8, 1024,
                                                                   &err));
    assert(err.empty());
    assert(sets.sets.size() == 2u);
    assert(sets.sets[0].field_indices.size() == 2u);
    assert(sets.sets[0].field_indices[0] == 0u);
    assert(sets.sets[0].field_indices[1] == 1u);
    assert(sets.sets[1].field_indices.size() == 1u);
    assert(sets.sets[1].field_indices[0] == 1u);
  }

  {
    std::string err;
    UsdcCrateFieldSetsTable sets{};
    assert(!freeusd::usd::crate::ReadUsdCrateFieldSetsTableFromPath(fixture("parity_tables.usdc"), sets, 1, 8, 1024,
                                                                    &err));
    assert(sets.sets.empty());
  }

  return 0;
}
