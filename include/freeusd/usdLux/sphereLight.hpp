#pragma once

#include <memory>

#include "freeusd/export.hpp"
#include "freeusd/gf/vec3f.hpp"
#include "freeusd/sdf/path.hpp"
#include "freeusd/usd/prim.hpp"

namespace freeusd::usd {
class Stage;
}

namespace freeusd::usdLux {

/// ``UsdLuxSphereLight``-shaped helper: read common sphere light inputs at a time code.
struct FREEUSD_API SphereLight {
  freeusd::usd::Prim prim;

  SphereLight() = default;
  explicit SphereLight(freeusd::usd::Prim p) : prim(std::move(p)) {}

  static SphereLight ReadFromPrim(const std::shared_ptr<const freeusd::usd::Stage>& stage,
                                  const freeusd::sdf::Path& path);

  explicit operator bool() const noexcept { return prim.IsValid(); }

  bool GetIntensity(float* out, double time = 1.0) const;
  bool GetColor(freeusd::gf::Vec3f* out, double time = 1.0) const;
  bool GetRadius(float* out, double time = 1.0) const;
};

}  // namespace freeusd::usdLux
