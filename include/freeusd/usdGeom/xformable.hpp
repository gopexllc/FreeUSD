#pragma once

#include "freeusd/gf/matrix4d.hpp"
#include "freeusd/usd/prim.hpp"

namespace freeusd::usdGeom {

/// Minimal ``Xformable`` wrapper: holds a @c Prim; transforms are identity until schema attrs are modeled.
struct Xformable {
  freeusd::usd::Prim prim;

  Xformable() = default;
  explicit Xformable(freeusd::usd::Prim p) : prim(std::move(p)) {}

  explicit operator bool() const noexcept { return prim.IsValid(); }
  const freeusd::usd::Prim& GetPrim() const noexcept { return prim; }

  /// Always identity in this build (no ``xformOp`` stack evaluation).
  freeusd::gf::Matrix4d ComputeLocalToWorldTransform() const { return freeusd::gf::Matrix4d::Identity(); }
};

}  // namespace freeusd::usdGeom
