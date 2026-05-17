#include "freeusd/usdShade/previewSurface.hpp"

#include "freeusd/usd/stage.hpp"

namespace freeusd::usdShade {

PreviewSurface PreviewSurface::ReadFromPrim(const std::shared_ptr<const freeusd::usd::Stage>& stage,
                                            const freeusd::sdf::Path& path) {
  return PreviewSurface(Shader::ReadFromPrim(stage, path));
}

bool PreviewSurface::IsPreviewSurface() const {
  return shader && shader.GetShaderId() == previewSurfaceTokens::id();
}

bool PreviewSurface::GetDiffuseColor(freeusd::gf::Vec3f* out, double time) const {
  return IsPreviewSurface() && shader.GetDiffuseColor(out, time);
}

bool PreviewSurface::GetMetallic(float* out, double time) const {
  return IsPreviewSurface() && shader.GetMetallic(out, time);
}

bool PreviewSurface::GetRoughness(float* out, double time) const {
  return IsPreviewSurface() && shader.GetRoughness(out, time);
}

bool PreviewSurface::GetOpacity(float* out, double time) const {
  return IsPreviewSurface() && shader.GetOpacity(out, time);
}

}  // namespace freeusd::usdShade
