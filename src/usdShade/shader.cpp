#include "freeusd/usdShade/shader.hpp"

#include <optional>

#include "freeusd/usd/stage.hpp"
#include "freeusd/usdShade/previewSurface.hpp"
#include "freeusd/usdShade/tokens.hpp"

namespace freeusd::usdShade {
namespace {

freeusd::tf::Token resolve_info_id_attr(const freeusd::usd::Prim& prim) {
  if (prim.HasAttribute(tokens::info_id())) {
    return tokens::info_id();
  }
  static const freeusd::tf::Token kTokenInfoId("token info:id");
  if (prim.HasAttribute(kTokenInfoId)) {
    return kTokenInfoId;
  }
  for (const std::string& name : prim.ListAttributeNames()) {
    if (name == "info:id" || (name.size() > 7 && name.compare(name.size() - 7, 7, "info:id") == 0)) {
      return freeusd::tf::Token(name);
    }
  }
  return {};
}

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

const freeusd::tf::Token& inputs_file_token() {
  static const freeusd::tf::Token kInputsFile("inputs:file");
  return kInputsFile;
}

bool prim_looks_like_shader(const freeusd::usd::Prim& prim) {
  if (!prim.IsValid()) {
    return false;
  }
  if (!resolve_info_id_attr(prim).IsEmpty()) {
    return true;
  }
  return prim.HasAttribute(previewSurfaceTokens::inputs_diffuseColor());
}

}  // namespace

Shader Shader::ReadFromPrim(const std::shared_ptr<const freeusd::usd::Stage>& stage, const freeusd::sdf::Path& path) {
  if (!stage) {
    return {};
  }
  return Shader(stage->GetPrimAtPath(path));
}

freeusd::tf::Token Shader::GetShaderId() const {
  if (!prim.IsValid()) {
    return {};
  }
  const freeusd::tf::Token attr = resolve_info_id_attr(prim);
  if (attr.IsEmpty()) {
    return {};
  }
  const freeusd::vt::Value v = prim.GetAttribute(attr, 1.0);
  freeusd::tf::Token id;
  if (v.GetToken(&id)) {
    return id;
  }
  std::string text;
  if (v.GetString(&text)) {
    return freeusd::tf::Token{text};
  }
  return {};
}

freeusd::vt::Value Shader::GetInput(const freeusd::tf::Token& input_name, double time) const {
  if (!prim.IsValid() || input_name.IsEmpty()) {
    return {};
  }
  return prim.GetAttribute(input_name, time);
}

bool Shader::GetDiffuseColor(freeusd::gf::Vec3f* out, double time) const {
  if (!out) {
    return false;
  }
  return GetInput(previewSurfaceTokens::inputs_diffuseColor(), time).GetVec3f(out);
}

bool Shader::GetMetallic(float* out, double time) const {
  if (!out) {
    return false;
  }
  return GetInput(previewSurfaceTokens::inputs_metallic(), time).GetFloat(out);
}

bool Shader::GetRoughness(float* out, double time) const {
  if (!out) {
    return false;
  }
  return GetInput(previewSurfaceTokens::inputs_roughness(), time).GetFloat(out);
}

bool Shader::GetOpacity(float* out, double time) const {
  if (!out) {
    return false;
  }
  return GetInput(previewSurfaceTokens::inputs_opacity(), time).GetFloat(out);
}

bool Shader::GetInputAssetPath(const freeusd::tf::Token& input_name, std::string* out_path, double time) const {
  if (!out_path || !prim.IsValid() || input_name.IsEmpty()) {
    return false;
  }
  out_path->clear();

  const freeusd::vt::Value direct = prim.GetAttribute(input_name, time);
  if (read_asset_path_from_value(direct, out_path)) {
    return true;
  }

  freeusd::sdf::Path target;
  if (!prim.GetAttributeConnectionTarget(input_name, &target) || target.IsEmpty()) {
    return false;
  }

  const std::shared_ptr<const freeusd::usd::Stage> stage = prim.GetStage();
  if (!stage) {
    return false;
  }
  const freeusd::usd::Prim connected = stage->GetPrimAtPath(target.GetPrimPath());
  if (!connected.IsValid()) {
    return false;
  }
  return read_asset_path_from_value(connected.GetAttribute(inputs_file_token(), time), out_path);
}

}  // namespace freeusd::usdShade
