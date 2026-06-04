#include <cassert>
#include <cmath>
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
  using freeusd::usd::crate::UsdcCrateTypedValueKind;
  using freeusd::usd::crate::UsdcCrateTypedValuesTable;
  using freeusd::usd::crate::UsdcCrateValuesTable;

  {
    std::string err;
    UsdcCrateTypedValuesTable typed{};
    assert(freeusd::usd::crate::ReadUsdCrateTypedValuesTableFromPath(fixture("parity_tables.usdc"), typed, 16, 1024,
                                                                    &err));
    assert(err.empty());
    assert(typed.entries.size() == 9u);
    assert(typed.entries[0].kind == UsdcCrateTypedValueKind::Int32);
    assert(typed.entries[0].int32_value == 42);
    assert(typed.entries[1].kind == UsdcCrateTypedValueKind::Float);
    assert(std::fabs(typed.entries[1].float_value - 1.5f) < 1e-5f);
    assert(typed.entries[2].kind == UsdcCrateTypedValueKind::TokenIndex);
    assert(typed.entries[2].token_index == 0u);
    assert(typed.entries[3].kind == UsdcCrateTypedValueKind::Bool);
    assert(typed.entries[3].bool_value);
    assert(typed.entries[4].kind == UsdcCrateTypedValueKind::Double);
    assert(std::fabs(typed.entries[4].double_value - 3.25) < 1e-12);
    assert(typed.entries[5].kind == UsdcCrateTypedValueKind::Int64);
    assert(typed.entries[5].int64_value == -9007199254740991LL);
    assert(typed.entries[6].kind == UsdcCrateTypedValueKind::StringUtf8);
    assert(typed.entries[6].string_utf8 == "parity");
    assert(typed.entries[7].kind == UsdcCrateTypedValueKind::Vec3f);
    assert(std::fabs(typed.entries[7].vec3f_value.data[0] - 1.0f) < 1e-5f);
    assert(std::fabs(typed.entries[7].vec3f_value.data[1] - 2.0f) < 1e-5f);
    assert(std::fabs(typed.entries[7].vec3f_value.data[2] - 3.0f) < 1e-5f);
    assert(typed.entries[8].kind == UsdcCrateTypedValueKind::StringIndex);
    assert(typed.entries[8].string_index == 1u);
  }

  {
    std::string err;
    UsdcCrateValuesTable opaque{};
    assert(freeusd::usd::crate::ReadUsdCrateValuesTableFromPath(fixture("parity_tables.usdc"), opaque, 16, 1024, &err));
    assert(err.empty());
    assert(opaque.entries.size() == 9u);
    assert(opaque.entries[0].bytes.size() == 4u);
  }

  {
    std::string err;
    UsdcCrateTypedValuesTable typed{};
    assert(!freeusd::usd::crate::ReadUsdCrateTypedValuesTableFromPath(fixture("parity_tables.usdc"), typed, 2, 1024,
                                                                     &err));
    assert(typed.entries.empty());
  }

  return 0;
}
