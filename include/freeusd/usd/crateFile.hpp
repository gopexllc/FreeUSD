
#pragma once

#include "freeusd/gf/quatd.hpp"
#include "freeusd/gf/quatf.hpp"
#include "freeusd/gf/vec2d.hpp"
#include "freeusd/gf/vec3d.hpp"
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "freeusd/export.hpp"
#include "freeusd/gf/vec2f.hpp"
#include "freeusd/gf/vec4f.hpp"
#include "freeusd/gf/vec3f.hpp"
#include "freeusd/sdf/path.hpp"

namespace freeusd::usd::crate {

/// Crate / USDC files use this identifier prefix at the start of the file (public format fact).
constexpr std::string_view UsdcCrateIdentifier() noexcept { return "PXR-USDC"; }

/// Fixed-size header at offset 0 after the published USDC crate layout (little-endian integers).
/// This is **not** a full crate decode: only the bootstrap prefix is interpreted.
struct UsdcCrateBootstrap {
  std::uint8_t file_version_major = 0;
  std::uint8_t file_version_minor = 0;
  std::uint8_t file_version_patch = 0;
  std::int64_t toc_byte_offset = 0;
};

/// Read the crate bootstrap from \p path (binary). Returns false on I/O or layout errors.
/// File must be at least 88 bytes and start with ``UsdcCrateIdentifier()``; integers are read **little-endian**.
FREEUSD_API bool ReadUsdCrateBootstrapFromPath(const std::string& path, UsdcCrateBootstrap& out,
                                               std::string* err_out = nullptr);

/// One on-disk USDC TOC section entry (``char name[16]`` + two little-endian ``int64``; **32** bytes total).
struct UsdcCrateTocSection {
  std::string name;
  std::int64_t start_byte_offset = 0;
  std::int64_t size_bytes = 0;
};

/// Parsed table-of-contents list at ``UsdcCrateBootstrap::toc_byte_offset`` (little-endian count + records).
/// This is **not** a full crate decode: section payloads are not read or interpreted.
struct UsdcCrateToc {
  std::uint64_t section_count = 0;
  std::vector<UsdcCrateTocSection> sections;
};

/// Reads the TOC after a successful bootstrap. Fails if ``section_count`` exceeds \p max_sections.
/// On-disk layout matches common **`.usdc`** crate files: **uint64_t** little-endian section count,
/// then ``section_count`` records of **16**-byte NUL-padded names plus two little-endian **int64** fields each.
/// Each section’s ``start_byte_offset`` / ``size_bytes`` must describe a byte range that fits in the file
/// (non-negative; ``start + size`` ≤ file size, with overflow-safe checks).
FREEUSD_API bool ReadUsdCrateTocFromPath(const std::string& path, UsdcCrateToc& out, std::size_t max_sections,
                                        std::string* err_out = nullptr);

/// Reads one TOC section payload by name into \p out_bytes. The payload is returned verbatim; no section-specific
/// decode is performed yet. Fails if the section is missing, exceeds \p max_section_bytes, or cannot be read fully.
FREEUSD_API bool ReadUsdCrateSectionBytesFromPath(const std::string& path, std::string_view section_name,
                                                  std::vector<std::uint8_t>& out_bytes, std::size_t max_section_bytes,
                                                  std::string* err_out = nullptr);

/// Reads a fixture-oriented `USDA` section payload as UTF-8 layer text.
/// This is a narrow bridge for USDA-first engine pipelines and not a full spec-level crate scene decode.
FREEUSD_API bool ReadUsdCrateUsdaSectionFromPath(const std::string& path, std::string& out_text,
                                                 std::size_t max_text_bytes, std::string* err_out = nullptr);

/// Validated low-level table payload: little-endian ``uint64_t`` count, then repeated ``uint64_t`` byte length + bytes.
struct UsdcCrateStringTable {
  std::vector<std::string> values;
};

/// Reads the ``STRINGS`` section using the shared fixture-oriented table payload shape documented above.
FREEUSD_API bool ReadUsdCrateStringTableFromPath(const std::string& path, UsdcCrateStringTable& out,
                                                 std::size_t max_entries, std::size_t max_total_bytes,
                                                 std::string* err_out = nullptr);
/// Reads the ``TOKENS`` section using the same validated table payload shape as @ref ReadUsdCrateStringTableFromPath.
FREEUSD_API bool ReadUsdCrateTokenTableFromPath(const std::string& path, UsdcCrateStringTable& out,
                                                std::size_t max_entries, std::size_t max_total_bytes,
                                                std::string* err_out = nullptr);

struct UsdcCratePathTable {
  std::vector<freeusd::sdf::Path> paths;
};

/// Reads the ``PATHS`` section using the same validated table payload shape as @ref ReadUsdCrateStringTableFromPath.
FREEUSD_API bool ReadUsdCratePathTableFromPath(const std::string& path, UsdcCratePathTable& out, std::size_t max_entries,
                                               std::size_t max_total_bytes, std::string* err_out = nullptr);

/// One decoded row from the fixture-oriented ``FIELDS`` table (indices into the ``TOKENS`` table).
struct UsdcCrateFieldEntry {
  std::uint64_t token_index = 0;
  std::uint64_t value_type_token_index = 0;
};

/// Validated ``FIELDS`` table: little-endian ``uint64_t`` count, then repeated token / value-type index pairs.
struct UsdcCrateFieldsTable {
  std::vector<UsdcCrateFieldEntry> entries;
};

/// Reads the ``FIELDS`` section using the shared fixture-oriented table payload shape documented above.
FREEUSD_API bool ReadUsdCrateFieldsTableFromPath(const std::string& path, UsdcCrateFieldsTable& out,
                                               std::size_t max_entries, std::size_t max_total_bytes,
                                               std::string* err_out = nullptr);

/// One decoded row from the fixture-oriented ``SPECS`` table (indices into ``PATHS`` / ``FIELDSETS`` tables).
struct UsdcCrateSpecEntry {
  std::uint64_t path_index = 0;
  std::uint64_t field_set_index = 0;
  std::uint64_t spec_type = 0;
};

/// Validated ``SPECS`` table: little-endian ``uint64_t`` count, then path / field-set / spec-type triples.
struct UsdcCrateSpecsTable {
  std::vector<UsdcCrateSpecEntry> entries;
};

/// Reads the ``SPECS`` section using the shared fixture-oriented table payload shape documented above.
FREEUSD_API bool ReadUsdCrateSpecsTableFromPath(const std::string& path, UsdcCrateSpecsTable& out,
                                                std::size_t max_entries, std::size_t max_total_bytes,
                                                std::string* err_out = nullptr);

/// One decoded field set: indices into the ``FIELDS`` table rows.
struct UsdcCrateFieldSet {
  std::vector<std::uint64_t> field_indices;
};

/// Validated ``FIELDSETS`` table: ``uint64_t`` set count, then per set a field count and index list.
struct UsdcCrateFieldSetsTable {
  std::vector<UsdcCrateFieldSet> sets;
};

/// Reads the ``FIELDSETS`` section using the shared fixture-oriented table payload shape documented above.
FREEUSD_API bool ReadUsdCrateFieldSetsTableFromPath(const std::string& path, UsdcCrateFieldSetsTable& out,
                                                    std::size_t max_field_sets, std::size_t max_fields_per_set,
                                                    std::size_t max_total_bytes, std::string* err_out = nullptr);

/// One opaque encoded value blob from the fixture-oriented ``VALUES`` table.
struct UsdcCrateValueEntry {
  std::vector<std::uint8_t> bytes;
};

/// Validated ``VALUES`` table: ``uint64_t`` count, then length-prefixed opaque byte payloads.
struct UsdcCrateValuesTable {
  std::vector<UsdcCrateValueEntry> entries;
};

/// Reads the ``VALUES`` section using the shared length-prefixed blob table shape (same as ``STRINGS`` payloads).
FREEUSD_API bool ReadUsdCrateValuesTableFromPath(const std::string& path, UsdcCrateValuesTable& out,
                                                 std::size_t max_entries, std::size_t max_total_bytes,
                                                 std::string* err_out = nullptr);

/// Fixture-oriented typed value kinds in the ``VALUES`` table (clean-room; not production USDC encoding).
enum class UsdcCrateTypedValueKind : std::uint64_t {
  Opaque = 0,
  Int32 = 1,
  Float = 2,
  TokenIndex = 3,
  Bool = 4,
  Double = 5,
  Int64 = 6,
  StringUtf8 = 7,
  Vec3f = 8,
  StringIndex = 9,
  Vec3d = 10,
  Int32Array = 11,
  FloatArray = 12,
  DoubleArray = 13,
  Vec2f = 14,
  Vec4f = 15,
  Vec2d = 16,
  Quatf = 17,
  Quatd = 18,
  TokenIndexArray = 19,
};

/// One decoded typed value from the fixture ``VALUES`` table.
struct UsdcCrateTypedValue {
  UsdcCrateTypedValueKind kind{UsdcCrateTypedValueKind::Opaque};
  std::vector<std::uint8_t> bytes;
  std::int32_t int32_value{0};
  float float_value{0.0f};
  std::uint64_t token_index{0};
  std::vector<std::uint64_t> token_index_array;
  bool bool_value{false};
  double double_value{0.0};
  std::int64_t int64_value{0};
  std::string string_utf8;
  freeusd::gf::Vec3f vec3f_value{};
  std::uint64_t string_index{0};
  freeusd::gf::Vec3d vec3d_value{};
  std::vector<std::int32_t> int32_array;
  std::vector<float> float_array;
  std::vector<double> double_array;
  freeusd::gf::Vec2f vec2f_value{};
  freeusd::gf::Vec4f vec4f_value{};
  freeusd::gf::Vec2d vec2d_value{};
  freeusd::gf::Quatf quatf_value{};
  freeusd::gf::Quatd quatd_value{};
};

struct UsdcCrateTypedValuesTable {
  std::vector<UsdcCrateTypedValue> entries;
};

/// Reads the fixture ``VALUES`` table: ``uint64_t`` count, then per entry kind, length, and payload bytes.
FREEUSD_API bool ReadUsdCrateTypedValuesTableFromPath(const std::string& path, UsdcCrateTypedValuesTable& out,
                                                      std::size_t max_entries, std::size_t max_total_bytes,
                                                      std::string* err_out = nullptr);

enum class UsdFileKind {
  /// Could not read path or empty file.
  IoOrEmpty = 0,
  /// Starts with ASCII ``#usda`` layer header (subset detection).
  UsdaAscii,
  /// Binary crate identifier ``PXR-USDC`` at offset 0 (no full crate parse).
  UsdcCrate,
  /// Readable bytes that are neither recognized USDA nor crate magic.
  Unknown,
};

/// Read the first bytes of \p path and classify common USD container kinds.
/// Full crate decode is **not** performed; \p UsdcCrate means “looks like a crate file”.
FREEUSD_API UsdFileKind DetectUsdFileKindFromPath(const std::string& path, std::string* detail_out = nullptr);

}  // namespace freeusd::usd::crate
