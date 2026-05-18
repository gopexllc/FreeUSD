#include "freeusd/usdLux/sphereLight.hpp"

#include "freeusd/usd/stage.hpp"
#include "freeusd/usdLux/tokens.hpp"

namespace freeusd::usdLux {

SphereLight SphereLight::ReadFromPrim(const std::shared_ptr<const freeusd::usd::Stage>& stage,
                                      const freeusd::sdf::Path& path) {
  if (!stage) {
    return {};
  }
  return SphereLight(stage->GetPrimAtPath(path));
}

bool SphereLight::GetIntensity(float* out, double time) const {
  if (!out || !prim.IsValid()) {
    return false;
  }
  return prim.GetAttribute(tokens::inputs_intensity(), time).GetFloat(out);
}

bool SphereLight::GetColor(freeusd::gf::Vec3f* out, double time) const {
  if (!out || !prim.IsValid()) {
    return false;
  }
  return prim.GetAttribute(tokens::inputs_color(), time).GetVec3f(out);
}

bool SphereLight::GetRadius(float* out, double time) const {
  if (!out || !prim.IsValid()) {
    return false;
  }
  return prim.GetAttribute(tokens::inputs_radius(), time).GetFloat(out);
}

}  // namespace freeusd::usdLux
