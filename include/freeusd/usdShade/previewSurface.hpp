#pragma once

#include "freeusd/export.hpp"
#include "freeusd/gf/vec3f.hpp"
#include "freeusd/tf/token.hpp"
#include "freeusd/usdShade/shader.hpp"

namespace freeusd::usdShade {

/// Published ``UsdPreviewSurface`` input attribute names (token strings only; clean-room).
namespace previewSurfaceTokens {

inline freeusd::tf::Token id() { return freeusd::tf::Token("UsdPreviewSurface"); }
inline freeusd::tf::Token inputs_diffuseColor() { return freeusd::tf::Token("inputs:diffuseColor"); }
inline freeusd::tf::Token inputs_metallic() { return freeusd::tf::Token("inputs:metallic"); }
inline freeusd::tf::Token inputs_roughness() { return freeusd::tf::Token("inputs:roughness"); }
inline freeusd::tf::Token inputs_opacity() { return freeusd::tf::Token("inputs:opacity"); }

}  // namespace previewSurfaceTokens

/// Thin wrapper over a ``Shader`` prim when ``info:id`` is ``UsdPreviewSurface``.
struct FREEUSD_API PreviewSurface {
  Shader shader;

  PreviewSurface() = default;
  explicit PreviewSurface(Shader s) : shader(std::move(s)) {}

  static PreviewSurface ReadFromPrim(const std::shared_ptr<const freeusd::usd::Stage>& stage,
                                     const freeusd::sdf::Path& path);

  explicit operator bool() const noexcept { return shader && IsPreviewSurface(); }
  bool IsPreviewSurface() const;

  bool GetDiffuseColor(freeusd::gf::Vec3f* out, double time = 1.0) const;
  bool GetMetallic(float* out, double time = 1.0) const;
  bool GetRoughness(float* out, double time = 1.0) const;
  bool GetOpacity(float* out, double time = 1.0) const;
};

}  // namespace freeusd::usdShade
