#pragma once

#include <string>
#include <string_view>

#include "freeusd/export.hpp"

namespace freeusd::usd::crate {

/// Crate / USDC files use this identifier prefix at the start of the file (public format fact).
constexpr std::string_view UsdcCrateIdentifier() noexcept { return "PXR-USDC"; }

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
