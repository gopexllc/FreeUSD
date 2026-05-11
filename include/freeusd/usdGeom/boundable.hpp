#pragma once

#include "freeusd/gf/bbox3d.hpp"
#include "freeusd/usd/prim.hpp"

namespace freeusd::usdGeom {

/// ``UsdGeomBoundable``-shaped helper for a tiny executable subset backed by authored scene data.
/// Currently understands scalar ``size`` (cube-like) and ``radius`` extents, then applies any
/// composed ``Xformable`` transform to produce a world-space box.
struct FREEUSD_API Boundable {
  freeusd::usd::Prim prim;

  Boundable() = default;
  explicit Boundable(freeusd::usd::Prim p) : prim(std::move(p)) {}

  explicit operator bool() const noexcept { return prim.IsValid(); }
  const freeusd::usd::Prim& GetPrim() const noexcept { return prim; }

  freeusd::gf::BBox3d ComputeLocalBound(double time = 1.0) const;
  freeusd::gf::BBox3d ComputeWorldBound(double time = 1.0) const;
};

}  // namespace freeusd::usdGeom
