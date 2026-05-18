#include "freeusd/usdLux/domeLight.hpp"

#include <optional>

#include "freeusd/usd/stage.hpp"
#include "freeusd/usdLux/tokens.hpp"

namespace freeusd::usdLux {
namespace {

std::optional<std::string> normalize_usd_asset_path_string(const std::string& s) {
  if (s.empty()) {
    return std::nullopt;
  }
  if (s.size() >= 2u && s.front() == '@' && s.back() == '@') {
    return s.substr(1, s.size() - 2u);
  }
  return s;
}

bool read_asset_path_from_value(const freeusd::vt::Value& v, std::string* out_path) {
  if (!out_path) {
    return false;
  }
  std::string text;
  if (!v.GetString(&text)) {
    return false;
  }
  const std::optional<std::string> normalized = normalize_usd_asset_path_string(text);
  if (!normalized || normalized->empty()) {
    return false;
  }
  *out_path = *normalized;
  return true;
}

}  // namespace

DomeLight DomeLight::ReadFromPrim(const std::shared_ptr<const freeusd::usd::Stage>& stage,
                                  const freeusd::sdf::Path& path) {
  if (!stage) {
    return {};
  }
  return DomeLight(stage->GetPrimAtPath(path));
}

bool DomeLight::GetIntensity(float* out, double time) const {
  if (!out || !prim.IsValid()) {
    return false;
  }
  return prim.GetAttribute(tokens::inputs_intensity(), time).GetFloat(out);
}

bool DomeLight::GetColor(freeusd::gf::Vec3f* out, double time) const {
  if (!out || !prim.IsValid()) {
    return false;
  }
  return prim.GetAttribute(tokens::inputs_color(), time).GetVec3f(out);
}

bool DomeLight::GetTextureFileAssetPath(std::string* out_path, double time) const {
  if (!out_path || !prim.IsValid()) {
    return false;
  }
  out_path->clear();
  return read_asset_path_from_value(prim.GetAttribute(tokens::inputs_texture_file(), time), out_path);
}

bool DomeLight::GetTextureFormat(std::string* out_format, double time) const {
  if (!out_format || !prim.IsValid()) {
    return false;
  }
  out_format->clear();
  const freeusd::vt::Value v = prim.GetAttribute(tokens::inputs_texture_format(), time);
  freeusd::tf::Token token;
  if (v.GetToken(&token) && !token.IsEmpty()) {
    *out_format = token.GetText();
    return true;
  }
  return v.GetString(out_format) && !out_format->empty();
}

}  // namespace freeusd::usdLux
