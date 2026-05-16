#pragma once

#include <string>
#include <vector>

#include "freeusd/export.hpp"
#include "freeusd/sdf/path.hpp"
#include "freeusd/usd/prim.hpp"
#include "freeusd/usdSkel/blendShape.hpp"

namespace freeusd::usdSkel {

/// Mesh binding for blend shapes: ``skel:blendShapes`` tokens plus ``skel:blendShapeTargets``.
///
/// glTF mapping: ``skel:blendShapes`` / ``blendShapeWeights`` on the geom (or remapped from
/// ``SkelAnimation``) correspond to ``mesh.weights``; each target path is a morph target slot.
struct FREEUSD_API SkelBlendShapes {
  freeusd::usd::Prim geom_prim;
  std::vector<std::string> blend_shape_tokens;
  std::vector<freeusd::sdf::Path> target_paths;

  SkelBlendShapes() = default;
  explicit SkelBlendShapes(freeusd::usd::Prim geom) : geom_prim(std::move(geom)) {}

  static SkelBlendShapes ReadFromGeomPrim(const freeusd::usd::Prim& geom);

  explicit operator bool() const noexcept {
    return geom_prim.IsValid() && !blend_shape_tokens.empty() && !target_paths.empty();
  }

  /// Per-token weights at @p time: local ``blendShapeWeights``, else remapped from ``skel:animationSource``.
  bool GetWeights(std::vector<float>* out, double time = 1.0) const;

  /// Resolved ``BlendShape`` prims in ``skel:blendShapes`` token order (invalid entries skipped).
  std::vector<BlendShape> ResolveTargets() const;
};

}  // namespace freeusd::usdSkel
