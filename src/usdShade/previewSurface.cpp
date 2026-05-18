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

bool PreviewSurface::GetEmissiveColor(freeusd::gf::Vec3f* out, double time) const {
  if (!IsPreviewSurface() || !out) {
    return false;
  }
  return shader.GetInput(previewSurfaceTokens::inputs_emissiveColor(), time).GetVec3f(out);
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

bool PreviewSurface::GetDiffuseTextureAssetPath(std::string* out_path, double time) const {
  if (!IsPreviewSurface() || !out_path) {
    return false;
  }
  return shader.GetInputAssetPath(previewSurfaceTokens::inputs_diffuseColor(), out_path, time);
}

bool PreviewSurface::GetNormalTextureAssetPath(std::string* out_path, double time) const {
  if (!IsPreviewSurface() || !out_path) {
    return false;
  }
  return shader.GetInputAssetPath(previewSurfaceTokens::inputs_normal(), out_path, time);
}

bool PreviewSurface::GetOcclusionTextureAssetPath(std::string* out_path, double time) const {
  if (!IsPreviewSurface() || !out_path) {
    return false;
  }
  return shader.GetInputAssetPath(previewSurfaceTokens::inputs_occlusion(), out_path, time);
}

bool PreviewSurface::GetMetallicTextureAssetPath(std::string* out_path, double time) const {
  if (!IsPreviewSurface() || !out_path) {
    return false;
  }
  return shader.GetInputAssetPath(previewSurfaceTokens::inputs_metallic(), out_path, time);
}

bool PreviewSurface::GetRoughnessTextureAssetPath(std::string* out_path, double time) const {
  if (!IsPreviewSurface() || !out_path) {
    return false;
  }
  return shader.GetInputAssetPath(previewSurfaceTokens::inputs_roughness(), out_path, time);
}

}  // namespace freeusd::usdShade
