#pragma once

#include "freeusd/export.hpp"
#include "freeusd/gf/vec3f.hpp"
#include "freeusd/sdf/path.hpp"
#include "freeusd/usd/prim.hpp"

namespace freeusd::usd {
class Stage;
}

namespace freeusd::usdPhysics {

/// ``PhysicsMassAPI``-shaped helper: read ``physics:density`` and ``physics:centerOfMass`` at a time code.
/// Detection uses composed attribute authorship or composed ``apiSchemas`` listing ``PhysicsMassAPI``.
struct FREEUSD_API MassAPI {
  freeusd::usd::Prim prim;

  MassAPI() = default;
  explicit MassAPI(freeusd::usd::Prim p) : prim(std::move(p)) {}

  static MassAPI ReadFromPrim(const std::shared_ptr<const freeusd::usd::Stage>& stage, const freeusd::sdf::Path& path);

  explicit operator bool() const noexcept { return prim.IsValid(); }

  bool IsMassAPI() const;

  bool GetDensity(float* out, double time = 1.0) const;
  bool GetCenterOfMass(freeusd::gf::Vec3f* out, double time = 1.0) const;
};

}  // namespace freeusd::usdPhysics
