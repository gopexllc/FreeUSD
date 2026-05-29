#pragma once

#include <vector>

#include "freeusd/export.hpp"
#include "freeusd/sdf/path.hpp"
#include "freeusd/usd/prim.hpp"

namespace freeusd::usd {
class Stage;
}

namespace freeusd::usdPhysics {

/// ``PhysicsFixedJoint``-shaped helper: read ``physics:body0`` / ``physics:body1`` and ``physics:jointEnabled``.
struct FREEUSD_API FixedJoint {
  freeusd::usd::Prim prim;

  FixedJoint() = default;
  explicit FixedJoint(freeusd::usd::Prim p) : prim(std::move(p)) {}

  static FixedJoint ReadFromPrim(const std::shared_ptr<const freeusd::usd::Stage>& stage,
                                 const freeusd::sdf::Path& path);

  explicit operator bool() const noexcept { return prim.IsValid(); }

  bool IsFixedJoint() const;

  bool GetBody0(freeusd::sdf::Path* out) const;
  bool GetBody1(freeusd::sdf::Path* out) const;
  bool GetJointEnabled(bool* out, double time = 1.0) const;
};

}  // namespace freeusd::usdPhysics
