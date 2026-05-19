#pragma once

#include <memory>
#include <vector>

#include "freeusd/export.hpp"
#include "freeusd/sdf/path.hpp"
#include "freeusd/usd/prim.hpp"
#include "freeusd/usdVol/openVdbAsset.hpp"

namespace freeusd::usd {
class Stage;
}

namespace freeusd::usdVol {

/// ``Volume``-shaped helper: read prim kind and composed ``field`` relationship targets.
struct FREEUSD_API Volume {
  freeusd::usd::Prim prim;

  Volume() = default;
  explicit Volume(freeusd::usd::Prim p) : prim(std::move(p)) {}

  static Volume ReadFromPrim(const std::shared_ptr<const freeusd::usd::Stage>& stage,
                             const freeusd::sdf::Path& path);

  explicit operator bool() const noexcept { return prim.IsValid(); }

  bool IsVolume() const;

  /// Composed ``field`` relationship targets (empty when not a volume or unauthored).
  std::vector<freeusd::sdf::Path> GetFieldRelationshipTargets() const;

  /// Field targets that resolve to ``OpenVDBAsset`` prims on the same stage.
  std::vector<OpenVDBAsset> GetOpenVDBFieldAssets() const;
};

}  // namespace freeusd::usdVol
