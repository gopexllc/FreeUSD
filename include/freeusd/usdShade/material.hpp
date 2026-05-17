#pragma once

#include <memory>
#include <string>
#include <vector>

#include "freeusd/export.hpp"
#include "freeusd/sdf/path.hpp"
#include "freeusd/usd/prim.hpp"

namespace freeusd::usd {
class Stage;
}

namespace freeusd::usdShade {

/// ``UsdShadeMaterial``-shaped helper: surface output connection and child shader prims.
struct FREEUSD_API Material {
  freeusd::usd::Prim prim;

  Material() = default;
  explicit Material(freeusd::usd::Prim p) : prim(std::move(p)) {}

  static Material ReadFromPrim(const std::shared_ptr<const freeusd::usd::Stage>& stage,
                               const freeusd::sdf::Path& path);

  explicit operator bool() const noexcept { return prim.IsValid(); }
  const freeusd::usd::Prim& GetPrim() const noexcept { return prim; }

  /// Prim path of the shader connected to ``outputs:surface`` (empty when unconnected).
  freeusd::sdf::Path GetSurfaceShaderPath() const;

  /// Direct child prims that look like shader nodes (authored ``info:id``).
  std::vector<freeusd::sdf::Path> ListShaderPrimPaths() const;
};

}  // namespace freeusd::usdShade
