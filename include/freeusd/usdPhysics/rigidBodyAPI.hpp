#pragma once

#include "freeusd/export.hpp"
#include "freeusd/sdf/path.hpp"
#include "freeusd/usd/prim.hpp"

namespace freeusd::usd {
class Stage;
}

namespace freeusd::usdPhysics {

/// ``PhysicsRigidBodyAPI``-shaped helper: read ``physics:mass`` at a time code.
/// Detection uses composed ``physics:mass`` authorship or composed ``apiSchemas`` listing ``PhysicsRigidBodyAPI``.
struct FREEUSD_API RigidBodyAPI {
  freeusd::usd::Prim prim;

  RigidBodyAPI() = default;
  explicit RigidBodyAPI(freeusd::usd::Prim p) : prim(std::move(p)) {}

  static RigidBodyAPI ReadFromPrim(const std::shared_ptr<const freeusd::usd::Stage>& stage,
                                   const freeusd::sdf::Path& path);

  explicit operator bool() const noexcept { return prim.IsValid(); }

  bool IsRigidBodyAPI() const;

  bool GetMass(float* out, double time = 1.0) const;
};

}  // namespace freeusd::usdPhysics
