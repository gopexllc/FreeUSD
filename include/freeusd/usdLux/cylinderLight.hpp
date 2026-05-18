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

/// ``UsdLuxCylinderLight``-shaped helper: read common cylinder light inputs at a time code.
struct FREEUSD_API CylinderLight {
  freeusd::usd::Prim prim;

  CylinderLight() = default;
  explicit CylinderLight(freeusd::usd::Prim p) : prim(std::move(p)) {}

  static CylinderLight ReadFromPrim(const std::shared_ptr<const freeusd::usd::Stage>& stage,
                                    const freeusd::sdf::Path& path);

  explicit operator bool() const noexcept { return prim.IsValid(); }

  bool GetIntensity(float* out, double time = 1.0) const;
  bool GetColor(freeusd::gf::Vec3f* out, double time = 1.0) const;
  bool GetLength(float* out, double time = 1.0) const;
  bool GetRadius(float* out, double time = 1.0) const;
};

}  // namespace freeusd::usdLux
