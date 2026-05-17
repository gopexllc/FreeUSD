#include <cassert>
#include <string>

#include "freeusd/usd/crateFile.hpp"

#ifndef FREEUSD_TEST_FIXTURES_DIR
#error "FREEUSD_TEST_FIXTURES_DIR must be set by CMake for usdc_fields_test"
#endif

namespace {

std::string fixture(const char* name) {
  return std::string(FREEUSD_TEST_FIXTURES_DIR) + "/" + name;
}

}  // namespace

int main() {
  using freeusd::usd::crate::UsdcCrateFieldEntry;
  using freeusd::usd::crate::UsdcCrateFieldsTable;

  {
    std::string err;
    UsdcCrateFieldsTable fields{};
    assert(freeusd::usd::crate::ReadUsdCrateFieldsTableFromPath(fixture("parity_tables.usdc"), fields, 8, 1024, &err));
    assert(err.empty());
    assert(fields.entries.size() == 2u);
    assert(fields.entries[0].token_index == 0u);
    assert(fields.entries[0].value_type_token_index == 1u);
    assert(fields.entries[1].token_index == 1u);
    assert(fields.entries[1].value_type_token_index == 0u);
  }

  {
    std::string err;
    UsdcCrateFieldsTable fields{};
    assert(!freeusd::usd::crate::ReadUsdCrateFieldsTableFromPath(fixture("parity_tables.usdc"), fields, 1, 1024, &err));
    assert(fields.entries.empty());
  }

  {
    std::string err;
    UsdcCrateFieldsTable fields{};
    assert(!freeusd::usd::crate::ReadUsdCrateFieldsTableFromPath(fixture("parity_tables.usdc") + ".missing", fields, 8,
                                                                 1024, &err));
    assert(fields.entries.empty());
  }

  return 0;
}
