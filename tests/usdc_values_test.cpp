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
    assert(freeusd::usd::crate::ReadUsdCrateTypedValuesTableFromPath(fixture("parity_tables.usdc"), typed, 19, 1024,
                                                                    &err));
    assert(err.empty());
    assert(typed.entries.size() == 18u);
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
    assert(typed.entries[9].kind == UsdcCrateTypedValueKind::Vec3d);
    assert(std::fabs(typed.entries[9].vec3d_value.data[0] - 4.0) < 1e-12);
    assert(std::fabs(typed.entries[9].vec3d_value.data[1] - 5.0) < 1e-12);
    assert(std::fabs(typed.entries[9].vec3d_value.data[2] - 6.0) < 1e-12);
    assert(typed.entries[10].kind == UsdcCrateTypedValueKind::Int32Array);
    assert(typed.entries[10].int32_array.size() == 3u);
    assert(typed.entries[10].int32_array[0] == 7);
    assert(typed.entries[10].int32_array[1] == 8);
    assert(typed.entries[10].int32_array[2] == 9);
    assert(typed.entries[11].kind == UsdcCrateTypedValueKind::FloatArray);
    assert(typed.entries[11].float_array.size() == 2u);
    assert(typed.entries[12].kind == UsdcCrateTypedValueKind::DoubleArray);
    assert(typed.entries[12].double_array.size() == 2u);
    assert(typed.entries[13].kind == UsdcCrateTypedValueKind::Vec2f);
    assert(typed.entries[13].vec2f_value.data[0] > 0.49f);
    assert(typed.entries[14].kind == UsdcCrateTypedValueKind::Vec4f);
    assert(std::fabs(typed.entries[11].float_array[0] - 0.25f) < 1e-5f);
    assert(std::fabs(typed.entries[11].float_array[1] - 0.75f) < 1e-5f);
    assert(std::fabs(typed.entries[14].vec4f_value.data[0] - 1.0f) < 1e-5f);
    assert(std::fabs(typed.entries[14].vec4f_value.data[1] - 2.0f) < 1e-5f);
    assert(std::fabs(typed.entries[14].vec4f_value.data[2] - 3.0f) < 1e-5f);
    assert(std::fabs(typed.entries[14].vec4f_value.data[3] - 4.0f) < 1e-5f);
    assert(typed.entries[15].kind == UsdcCrateTypedValueKind::Vec2d);
    assert(std::fabs(typed.entries[15].vec2d_value.data[0] - 0.5) < 1e-12);
    assert(std::fabs(typed.entries[15].vec2d_value.data[1] - 1.75) < 1e-12);
    assert(typed.entries[16].kind == UsdcCrateTypedValueKind::Quatf);
    assert(std::fabs(typed.entries[16].quatf_value.real - 1.0f) < 1e-5f);
    assert(std::fabs(typed.entries[16].quatf_value.i - 0.5f) < 1e-5f);
    assert(std::fabs(typed.entries[16].quatf_value.j - 0.25f) < 1e-5f);
    assert(std::fabs(typed.entries[16].quatf_value.k - 0.125f) < 1e-5f);
    assert(typed.entries[17].kind == UsdcCrateTypedValueKind::Quatd);
    assert(std::fabs(typed.entries[17].quatd_value.real - 1.0) < 1e-12);
    assert(std::fabs(typed.entries[17].quatd_value.i - 0.5) < 1e-12);
    assert(std::fabs(typed.entries[17].quatd_value.j - 0.25) < 1e-12);
    assert(std::fabs(typed.entries[17].quatd_value.k - 0.125) < 1e-12);

    freeusd::usd::crate::UsdcCrateStringTable strings{};
    assert(freeusd::usd::crate::ReadUsdCrateStringTableFromPath(fixture("parity_tables.usdc"), strings, 16, 1024, &err));
    assert(strings.values.size() >= 2u);
    assert(strings.values[typed.entries[8].string_index] == "world");
  }

  {
    std::string err;
    UsdcCrateValuesTable opaque{};
    assert(freeusd::usd::crate::ReadUsdCrateValuesTableFromPath(fixture("parity_tables.usdc"), opaque, 19, 1024, &err));
    assert(err.empty());
    assert(opaque.entries.size() == 18u);
    assert(opaque.entries[0].bytes.size() == 4u);
  }

  {
    std::string err;
    UsdcCrateTypedValuesTable typed{};
    assert(!freeusd::usd::crate::ReadUsdCrateTypedValuesTableFromPath(fixture("parity_tables.usdc"), typed, 2, 1024,
                                                                     &err));
    assert(typed.entries.empty());
  }

  {
    std::string err;
    UsdcCrateTypedValuesTable zlib_typed{};
    assert(freeusd::usd::crate::ReadUsdCrateTypedValuesTableFromPath(fixture("parity_tables_zlib.usdc"), zlib_typed,
                                                                    19, 1024, &err));
    assert(err.empty());
    assert(zlib_typed.entries.size() == 18u);
    assert(zlib_typed.entries[0].int32_value == 42);
    assert(zlib_typed.entries[11].float_array.size() == 2u);
    assert(zlib_typed.entries[14].kind == UsdcCrateTypedValueKind::Vec4f);
  }

  {
    std::string err;
    UsdcCrateTypedValuesTable lz4_typed{};
    assert(freeusd::usd::crate::ReadUsdCrateTypedValuesTableFromPath(fixture("parity_tables_lz4.usdc"), lz4_typed, 19,
                                                                    1024, &err));
    assert(err.empty());
    assert(lz4_typed.entries.size() == 18u);
    assert(lz4_typed.entries[14].vec4f_value.data[3] > 3.99f);
  }

  return 0;
}
