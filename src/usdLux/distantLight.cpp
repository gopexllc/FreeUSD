#include "freeusd/usdLux/distantLight.hpp"

#include "freeusd/usd/stage.hpp"
#include "freeusd/usdLux/tokens.hpp"

namespace freeusd::usdLux {

DistantLight DistantLight::ReadFromPrim(const std::shared_ptr<const freeusd::usd::Stage>& stage,
                                        const freeusd::sdf::Path& path) {
  if (!stage) {
    return {};
  }
  return DistantLight(stage->GetPrimAtPath(path));
}

bool DistantLight::GetIntensity(float* out, double time) const {
  if (!out || !prim.IsValid()) {
    return false;
  }
  return prim.GetAttribute(tokens::inputs_intensity(), time).GetFloat(out);
}

bool DistantLight::GetColor(freeusd::gf::Vec3f* out, double time) const {
  if (!out || !prim.IsValid()) {
    return false;
  }
  return prim.GetAttribute(tokens::inputs_color(), time).GetVec3f(out);
}

bool DistantLight::GetAngle(float* out, double time) const {
  if (!out || !prim.IsValid()) {
    return false;
  }
  return prim.GetAttribute(tokens::inputs_angle(), time).GetFloat(out);
}

}  // namespace freeusd::usdLux
