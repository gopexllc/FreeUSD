#pragma once

#include <string>

#include "freeusd/export.hpp"

namespace freeusd::ar {

/// Maximum bytes read from a single on-disk USDA layer file (DoS guard for ``LoadFromFile``).
constexpr std::size_t kMaxUsdaLayerFileBytes = 64u * 1024u * 1024u;

/// Maximum bytes for a USDC crate file used by bootstrap/TOC/section readers.
constexpr std::size_t kMaxUsdcCrateFileBytes = 256u * 1024u * 1024u;

/// ``weakly_canonical`` of @p path, or empty on failure.
FREEUSD_API std::string CanonicalizeFilesystemPath(const std::string& path);

/// True when @p resolved_canonical is the same as or under @p root_canonical (both must already be canonical).
FREEUSD_API bool PathIsWithinRootDirectory(const std::string& resolved_canonical, const std::string& root_canonical);

/// Canonicalize @p authored relative to @p anchor_directory; returns empty if resolution fails or escapes the anchor.
FREEUSD_API std::string ResolvePathUnderAnchor(const std::string& anchor_directory, const std::string& authored_path);

}  // namespace freeusd::ar
