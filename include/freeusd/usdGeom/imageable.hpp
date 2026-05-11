#pragma once

#include <string>

#include "freeusd/export.hpp"
#include "freeusd/usd/prim.hpp"

namespace freeusd::usdGeom {

/// ``UsdGeomImageable``-shaped helper for inherited `visibility` and `purpose` queries.
struct FREEUSD_API Imageable {
  freeusd::usd::Prim prim;

  Imageable() = default;
  explicit Imageable(freeusd::usd::Prim p) : prim(std::move(p)) {}

  explicit operator bool() const noexcept { return prim.IsValid(); }
  const freeusd::usd::Prim& GetPrim() const noexcept { return prim; }

  /// True unless this prim or an ancestor authors `visibility = invisible`.
  bool ComputeVisibility(double time = 1.0) const;

  /// Inherited `purpose` token-like value, defaulting to `"default"` when no stronger purpose is authored.
  std::string ComputePurpose(double time = 1.0) const;
};

}  // namespace freeusd::usdGeom
