#include "freeusd/usdLux/diskLight.hpp"

#include "freeusd/usd/stage.hpp"
#include "freeusd/usdLux/tokens.hpp"

namespace freeusd::usdLux {

DiskLight DiskLight::ReadFromPrim(const std::shared_ptr<const freeusd::usd::Stage>& stage,
                                  const freeusd::sdf::Path& path) {
  if (!stage) {
    return {};
  }
  return DiskLight(stage->GetPrimAtPath(path));
}

bool DiskLight::GetIntensity(float* out, double time) const {
  if (!out || !prim.IsValid()) {
    return false;
  }
  return prim.GetAttribute(tokens::inputs_intensity(), time).GetFloat(out);
}

bool DiskLight::GetColor(freeusd::gf::Vec3f* out, double time) const {
  if (!out || !prim.IsValid()) {
    return false;
  }
  return prim.GetAttribute(tokens::inputs_color(), time).GetVec3f(out);
}

bool DiskLight::GetRadius(float* out, double time) const {
  if (!out || !prim.IsValid()) {
    return false;
  }
  return prim.GetAttribute(tokens::inputs_radius(), time).GetFloat(out);
}

}  // namespace freeusd::usdLux
