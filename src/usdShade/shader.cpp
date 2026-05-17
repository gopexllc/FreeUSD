#include "freeusd/usdShade/shader.hpp"

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

}  // namespace freeusd::usdShade
