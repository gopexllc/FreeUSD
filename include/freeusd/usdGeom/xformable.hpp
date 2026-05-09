#pragma once

#include "freeusd/gf/matrix4d.hpp"
#include "freeusd/usd/prim.hpp"

namespace freeusd::usdGeom {

/// Placeholder for Xformable utilities (UsdGeomXformable parity is future work).
struct Xformable {
  freeusd::usd::Prim prim;
  freeusd::gf::Matrix4d ComputeLocalToWorld() const { return freeusd::gf::Matrix4d::Identity(); }
};

}  // namespace freeusd::usdGeom
