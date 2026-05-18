#include "freeusd/usdLux/cylinderLight.hpp"

#include "freeusd/usd/stage.hpp"
#include "freeusd/usdLux/tokens.hpp"

namespace freeusd::usdLux {

CylinderLight CylinderLight::ReadFromPrim(const std::shared_ptr<const freeusd::usd::Stage>& stage,
                                          const freeusd::sdf::Path& path) {
  if (!stage) {
    return {};
  }
  return CylinderLight(stage->GetPrimAtPath(path));
}

bool CylinderLight::GetIntensity(float* out, double time) const {
  if (!out || !prim.IsValid()) {
    return false;
  }
  return prim.GetAttribute(tokens::inputs_intensity(), time).GetFloat(out);
}

bool CylinderLight::GetColor(freeusd::gf::Vec3f* out, double time) const {
  if (!out || !prim.IsValid()) {
    return false;
  }
  return prim.GetAttribute(tokens::inputs_color(), time).GetVec3f(out);
}

bool CylinderLight::GetLength(float* out, double time) const {
  if (!out || !prim.IsValid()) {
    return false;
  }
  return prim.GetAttribute(tokens::inputs_length(), time).GetFloat(out);
}

bool CylinderLight::GetRadius(float* out, double time) const {
  if (!out || !prim.IsValid()) {
    return false;
  }
  return prim.GetAttribute(tokens::inputs_radius(), time).GetFloat(out);
}

}  // namespace freeusd::usdLux
