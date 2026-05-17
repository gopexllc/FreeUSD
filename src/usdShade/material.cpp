#include "freeusd/usdShade/material.hpp"

#include <vector>

#include "freeusd/usd/stage.hpp"
#include "freeusd/usdShade/previewSurface.hpp"
#include "freeusd/usdShade/tokens.hpp"

namespace freeusd::usdShade {
namespace {

bool prim_looks_like_shader(const freeusd::usd::Prim& prim) {
  if (!prim.IsValid()) {
    return false;
  }
  if (prim.HasAttribute(tokens::info_id())) {
    return true;
  }
  static const freeusd::tf::Token kTokenInfoId("token info:id");
  if (prim.HasAttribute(kTokenInfoId)) {
    return true;
  }
  for (const std::string& name : prim.ListAttributeNames()) {
    if (name == "info:id" || (name.size() > 7 && name.compare(name.size() - 7, 7, "info:id") == 0)) {
      return true;
    }
  }
  return prim.HasAttribute(previewSurfaceTokens::inputs_diffuseColor());
}

}  // namespace

Material Material::ReadFromPrim(const std::shared_ptr<const freeusd::usd::Stage>& stage,
                                const freeusd::sdf::Path& path) {
  if (!stage) {
    return {};
  }
  return Material(stage->GetPrimAtPath(path));
}

freeusd::sdf::Path Material::GetSurfaceShaderPath() const {
  if (!prim.IsValid()) {
    return {};
  }
  freeusd::sdf::Path target;
  if (!prim.GetAttributeConnectionTarget(tokens::outputs_surface(), &target)) {
    return {};
  }
  if (!target.IsPropertyPath()) {
    return {};
  }
  return target.GetPrimPath();
}

std::vector<freeusd::sdf::Path> Material::ListShaderPrimPaths() const {
  std::vector<freeusd::sdf::Path> out;
  if (!prim.IsValid()) {
    return out;
  }
  for (const freeusd::usd::Prim& child : prim.GetChildren()) {
    if (!child.IsValid()) {
      continue;
    }
    if (prim_looks_like_shader(child)) {
      out.push_back(child.GetPath());
    }
  }
  return out;
}

}  // namespace freeusd::usdShade
