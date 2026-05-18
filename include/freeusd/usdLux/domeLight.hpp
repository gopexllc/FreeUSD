#pragma once

#include <memory>
#include <string>

#include "freeusd/export.hpp"
#include "freeusd/gf/vec3f.hpp"
#include "freeusd/sdf/path.hpp"
#include "freeusd/usd/prim.hpp"

namespace freeusd::usd {
class Stage;
}

namespace freeusd::usdLux {

/// ``UsdLuxDomeLight``-shaped helper: read common dome light inputs at a time code.
struct FREEUSD_API DomeLight {
  freeusd::usd::Prim prim;

  DomeLight() = default;
  explicit DomeLight(freeusd::usd::Prim p) : prim(std::move(p)) {}

  static DomeLight ReadFromPrim(const std::shared_ptr<const freeusd::usd::Stage>& stage,
                                const freeusd::sdf::Path& path);

  explicit operator bool() const noexcept { return prim.IsValid(); }

  bool GetIntensity(float* out, double time = 1.0) const;
  bool GetColor(freeusd::gf::Vec3f* out, double time = 1.0) const;
  bool GetTextureFileAssetPath(std::string* out_path, double time = 1.0) const;
  bool GetTextureFormat(std::string* out_format, double time = 1.0) const;
};

}  // namespace freeusd::usdLux
