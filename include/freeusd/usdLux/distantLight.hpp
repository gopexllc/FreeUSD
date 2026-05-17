#pragma once

#include "freeusd/export.hpp"
#include "freeusd/gf/vec3f.hpp"
#include "freeusd/sdf/path.hpp"
#include "freeusd/usd/prim.hpp"

namespace freeusd::usd {
class Stage;
}

namespace freeusd::usdLux {

/// ``UsdLuxDistantLight``-shaped helper: read common light inputs at a time code.
struct FREEUSD_API DistantLight {
  freeusd::usd::Prim prim;

  DistantLight() = default;
  explicit DistantLight(freeusd::usd::Prim p) : prim(std::move(p)) {}

  static DistantLight ReadFromPrim(const std::shared_ptr<const freeusd::usd::Stage>& stage,
                                   const freeusd::sdf::Path& path);

  explicit operator bool() const noexcept { return prim.IsValid(); }

  bool GetIntensity(float* out, double time = 1.0) const;
  bool GetColor(freeusd::gf::Vec3f* out, double time = 1.0) const;
  bool GetAngle(float* out, double time = 1.0) const;
};

}  // namespace freeusd::usdLux
