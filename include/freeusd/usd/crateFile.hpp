#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "freeusd/export.hpp"

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
