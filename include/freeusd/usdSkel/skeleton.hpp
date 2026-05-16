#pragma once

#include <memory>
#include <string>
#include <vector>

#include "freeusd/export.hpp"
#include "freeusd/gf/matrix4d.hpp"
#include "freeusd/sdf/path.hpp"
#include "freeusd/usd/prim.hpp"

namespace freeusd::usd {
class Stage;
}

namespace freeusd::usdSkel {

/// ``UsdSkelSkeleton``-shaped helper: reads ``joints``, ``bindTransforms``, and ``restTransforms``.
struct FREEUSD_API Skeleton {
  freeusd::usd::Prim prim;

  Skeleton() = default;
  explicit Skeleton(freeusd::usd::Prim p) : prim(std::move(p)) {}

  /// Load skeleton fields from a composed stage prim (invalid prim when absent).
  static Skeleton ReadFromPrim(const std::shared_ptr<const freeusd::usd::Stage>& stage,
                               const freeusd::sdf::Path& path);

  explicit operator bool() const noexcept { return prim.IsValid(); }
  const freeusd::usd::Prim& GetPrim() const noexcept { return prim; }

  /// Joint path tokens in bind order (matches glTF ``skin.joints`` ordering when paths are authored).
  std::vector<std::string> GetJointNames() const;

  /// Parent index per joint from path hierarchy; root joints use ``-1``.
  std::vector<int> GetJointParentIndices() const;

  bool GetBindTransforms(std::vector<freeusd::gf::Matrix4d>* out, double time = 1.0) const;
  bool GetRestTransforms(std::vector<freeusd::gf::Matrix4d>* out, double time = 1.0) const;

  /// World-space bind matrices from authored bind transforms and parent indices.
  std::vector<freeusd::gf::Matrix4d> ComputeWorldBindMatrices(double time = 1.0) const;
};

}  // namespace freeusd::usdSkel
