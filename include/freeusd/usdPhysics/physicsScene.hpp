#pragma once

#include "freeusd/export.hpp"
#include "freeusd/gf/vec3f.hpp"
#include "freeusd/sdf/path.hpp"
#include "freeusd/usd/prim.hpp"

namespace freeusd::usd {
class Stage;
}

namespace freeusd::usdPhysics {

/// ``UsdPhysicsScene``-shaped helper: read common scene physics inputs at a time code.
struct FREEUSD_API PhysicsScene {
  freeusd::usd::Prim prim;

  PhysicsScene() = default;
  explicit PhysicsScene(freeusd::usd::Prim p) : prim(std::move(p)) {}

  static PhysicsScene ReadFromPrim(const std::shared_ptr<const freeusd::usd::Stage>& stage,
                                   const freeusd::sdf::Path& path);

  explicit operator bool() const noexcept { return prim.IsValid(); }

  bool IsPhysicsScene() const;

  bool GetGravityDirection(freeusd::gf::Vec3f* out, double time = 1.0) const;
  bool GetGravityMagnitude(float* out, double time = 1.0) const;
};

}  // namespace freeusd::usdPhysics
