#pragma once

#include <memory>
#include <optional>
#include <string>

#include "freeusd/export.hpp"
#include "freeusd/gf/vec3f.hpp"
#include "freeusd/sdf/path.hpp"
#include "freeusd/tf/token.hpp"
#include "freeusd/usd/prim.hpp"
#include "freeusd/vt/value.hpp"

namespace freeusd::usd {
class Stage;
}

namespace freeusd::usdShade {

/// ``UsdShadeShader``-shaped helper: ``info:id`` and evaluated shader inputs.
struct FREEUSD_API Shader {
  freeusd::usd::Prim prim;

  Shader() = default;
  explicit Shader(freeusd::usd::Prim p) : prim(std::move(p)) {}

  static Shader ReadFromPrim(const std::shared_ptr<const freeusd::usd::Stage>& stage,
                             const freeusd::sdf::Path& path);

  explicit operator bool() const noexcept { return prim.IsValid(); }
  const freeusd::usd::Prim& GetPrim() const noexcept { return prim; }

  /// Shader implementation id (e.g. ``UsdPreviewSurface``).
  freeusd::tf::Token GetShaderId() const;

  /// Evaluated input at @p time (follows attribute connections via the stage).
  freeusd::vt::Value GetInput(const freeusd::tf::Token& input_name, double time = 1.0) const;

  bool GetDiffuseColor(freeusd::gf::Vec3f* out, double time = 1.0) const;
  bool GetMetallic(float* out, double time = 1.0) const;
  bool GetRoughness(float* out, double time = 1.0) const;
  bool GetOpacity(float* out, double time = 1.0) const;

  /// Resolved asset path for a shader input (direct ``asset`` value or one connection hop to ``inputs:file``).
  bool GetInputAssetPath(const freeusd::tf::Token& input_name, std::string* out_path, double time = 1.0) const;
};

}  // namespace freeusd::usdShade
