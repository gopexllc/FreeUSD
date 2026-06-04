#pragma once

#include <memory>
#include <optional>

#include "freeusd/export.hpp"
#include "freeusd/sdf/path.hpp"
#include "freeusd/usd/prim.hpp"
#include "freeusd/usdSkel/skelAnimation.hpp"
#include "freeusd/usdSkel/skeleton.hpp"

namespace freeusd::usd {
class Stage;
}

namespace freeusd::usdSkel {

/// ``UsdSkelRoot``-shaped helper: locates skeleton and optional animation source under a scope.
struct FREEUSD_API SkelRoot {
  freeusd::usd::Prim prim;

  SkelRoot() = default;
  explicit SkelRoot(freeusd::usd::Prim p) : prim(std::move(p)) {}

  static SkelRoot ReadFromPrim(const std::shared_ptr<const freeusd::usd::Stage>& stage,
                               const freeusd::sdf::Path& path);

  explicit operator bool() const noexcept { return prim.IsValid(); }

  /// First descendant ``Skeleton`` prim (depth-first).
  std::optional<freeusd::sdf::Path> FindSkeletonPath() const;

  /// Descendant prims with a resolved ``skel:skeleton`` binding (depth-first).
  std::vector<freeusd::sdf::Path> FindBoundGeomPaths() const;

  /// ``skel:animationSource`` relationship or token when authored.
  std::optional<freeusd::sdf::Path> GetAnimationSourcePath() const;

  Skeleton GetSkeleton() const;
  SkelAnimation GetAnimationSource() const;
};

}  // namespace freeusd::usdSkel
