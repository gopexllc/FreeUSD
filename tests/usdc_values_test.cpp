#include <cassert>
#include <string>

#include "freeusd/usd/crateFile.hpp"

#ifndef FREEUSD_TEST_FIXTURES_DIR
#error "FREEUSD_TEST_FIXTURES_DIR must be set by CMake for usdc_values_test"
#endif

namespace {

std::string fixture(const char* name) {
  return std::string(FREEUSD_TEST_FIXTURES_DIR) + "/" + name;
}

}  // namespace

int main() {
  using freeusd::usd::crate::UsdcCrateValuesTable;

  {
    std::string err;
    UsdcCrateValuesTable values{};
    assert(freeusd::usd::crate::ReadUsdCrateValuesTableFromPath(fixture("parity_tables.usdc"), values, 8, 1024,
                                                              &err));
    assert(err.empty());
    assert(values.entries.size() == 2u);
    assert(values.entries[0].bytes.size() == 2u);
    assert(values.entries[0].bytes[0] == 'v' && values.entries[0].bytes[1] == '0');
    assert(values.entries[1].bytes.size() == 10u);
    assert(values.entries[1].bytes[0] == 'v' && values.entries[1].bytes[1] == '1');
  }

  {
    std::string err;
    UsdcCrateValuesTable values{};
    assert(!freeusd::usd::crate::ReadUsdCrateValuesTableFromPath(fixture("parity_tables.usdc"), values, 1, 1024,
                                                                 &err));
    assert(values.entries.empty());
  }

  return 0;
}
