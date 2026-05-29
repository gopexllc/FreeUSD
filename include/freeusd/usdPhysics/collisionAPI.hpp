#pragma once

#include "freeusd/export.hpp"
#include "freeusd/sdf/path.hpp"
#include "freeusd/usd/prim.hpp"

namespace freeusd::usd {
class Stage;
}

namespace freeusd::usdPhysics {

/// ``PhysicsCollisionAPI``-shaped helper: read ``physics:collisionEnabled`` at a time code.
/// Detection uses composed ``physics:collisionEnabled`` authorship or composed ``apiSchemas`` listing
/// ``PhysicsCollisionAPI``.
struct FREEUSD_API CollisionAPI {
  freeusd::usd::Prim prim;

  CollisionAPI() = default;
  explicit CollisionAPI(freeusd::usd::Prim p) : prim(std::move(p)) {}

  static CollisionAPI ReadFromPrim(const std::shared_ptr<const freeusd::usd::Stage>& stage,
                                   const freeusd::sdf::Path& path);

  explicit operator bool() const noexcept { return prim.IsValid(); }

  bool IsCollisionAPI() const;

  bool GetCollisionEnabled(bool* out, double time = 1.0) const;
};

}  // namespace freeusd::usdPhysics
