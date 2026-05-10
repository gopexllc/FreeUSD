#pragma once

#include <optional>
#include <string>
#include <string_view>

#include "freeusd/export.hpp"
#include "freeusd/sdf/path.hpp"

namespace freeusd::sdf {

/// Authored composed reference (@asset@ with optional trailing absolute prim path literal).
struct FREEUSD_API PrimReference {
  std::string asset_path{};
  std::optional<Path> prim_path{};

  bool IsEmpty() const noexcept { return asset_path.empty(); }

  /// Populate from a USDA/snippet fragment: \@path\@, \@path\@\<\/Prim\>, a quoted asset, or bare token path.
  static bool ParseAuthored(std::string_view text, PrimReference* out);

  /// Canonical \@asset\@\[optional \<\/path\>] string for USDA I/O (no extra spaces).
  std::string FormatAuthoredForUsda() const;
};

}  // namespace freeusd::sdf
