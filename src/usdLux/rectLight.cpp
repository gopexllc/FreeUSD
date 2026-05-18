#include "freeusd/usdLux/rectLight.hpp"

#include "freeusd/usd/stage.hpp"
#include "freeusd/usdLux/tokens.hpp"

namespace freeusd::usdLux {

RectLight RectLight::ReadFromPrim(const std::shared_ptr<const freeusd::usd::Stage>& stage,
                                  const freeusd::sdf::Path& path) {
  if (!stage) {
    return {};
  }
  return RectLight(stage->GetPrimAtPath(path));
}

bool RectLight::GetIntensity(float* out, double time) const {
  if (!out || !prim.IsValid()) {
    return false;
  }
  return prim.GetAttribute(tokens::inputs_intensity(), time).GetFloat(out);
}

bool RectLight::GetColor(freeusd::gf::Vec3f* out, double time) const {
  if (!out || !prim.IsValid()) {
    return false;
  }
  return prim.GetAttribute(tokens::inputs_color(), time).GetVec3f(out);
}

bool RectLight::GetWidth(float* out, double time) const {
  if (!out || !prim.IsValid()) {
    return false;
  }
  return prim.GetAttribute(tokens::inputs_width(), time).GetFloat(out);
}

bool RectLight::GetHeight(float* out, double time) const {
  if (!out || !prim.IsValid()) {
    return false;
  }
  return prim.GetAttribute(tokens::inputs_height(), time).GetFloat(out);
}

}  // namespace freeusd::usdLux
