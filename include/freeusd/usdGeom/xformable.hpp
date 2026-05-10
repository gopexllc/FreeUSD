#pragma once

#include "freeusd/gf/matrix4d.hpp"
#include "freeusd/usd/prim.hpp"

namespace freeusd::usdGeom {

/// ``UsdGeomXformable``-shaped helper: reads a small subset of ``xformOp`` attributes and walks the prim chain.
///
/// Supported today: ``xformOp:translate`` and ``xformOp:scale`` as ``double3`` / ``vec3d`` values, applied in
/// ``xformOpOrder`` when that attribute is a comma-separated list of op names; otherwise translate then scale if
/// authored. Rotations and other ops are ignored (identity) until extended.
struct FREEUSD_API Xformable {
  freeusd::usd::Prim prim;

  Xformable() = default;
  explicit Xformable(freeusd::usd::Prim p) : prim(std::move(p)) {}

  explicit operator bool() const noexcept { return prim.IsValid(); }
  const freeusd::usd::Prim& GetPrim() const noexcept { return prim; }

  /// Local transform from authored ``xformOp``\ s at \p time (defaults to ``1.0``).
  freeusd::gf::Matrix4d ComputeLocalTransform(double time = 1.0) const;

  /// Concatenation of ancestor locals from root toward \p prim (same \p time).
  freeusd::gf::Matrix4d ComputeLocalToWorldTransform(double time = 1.0) const;
};

}  // namespace freeusd::usdGeom
