#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include "freeusd/export.hpp"
#include "freeusd/gf/vec3f.hpp"
#include "freeusd/sdf/path.hpp"
#include "freeusd/usd/prim.hpp"
#include "freeusd/usdSkel/blendShape.hpp"

namespace freeusd::usd {
class Stage;
}

namespace freeusd::usdSkel {

/// Resolved blend-shape target on a skinnable geom prim.
struct FREEUSD_API MorphTargetBinding {
  std::string token;
  freeusd::sdf::Path target_path{};
  float weight{0.0F};
};

/// Aggregates ``skel:blendShapes`` / ``skel:blendShapeTargets`` / ``blendShapeWeights`` on a geom prim.
///
/// glTF mapping: ``mesh.weights`` + morph target POSITION deltas ↔ blend-shape weights + ``offsets``.
struct FREEUSD_API MorphTargets {
  freeusd::usd::Prim geom_prim;

  MorphTargets() = default;
  explicit MorphTargets(freeusd::usd::Prim geom) : geom_prim(std::move(geom)) {}

  static MorphTargets ReadFromGeomPrim(const freeusd::usd::Prim& geom);

  explicit operator bool() const noexcept { return geom_prim.IsValid(); }

  /// ``skel:blendShapes`` token order (maps weight vector indices).
  std::vector<std::string> GetBlendShapeTokens() const;

  /// ``skel:blendShapeTargets`` relationship paths (parallel to tokens; entries may be empty).
  std::vector<freeusd::sdf::Path> GetBlendShapeTargetPaths() const;

  /// Weights at @p time (local ``blendShapeWeights`` or remapped from ``skel:animationSource``).
  bool GetWeights(std::vector<float>* out, double time = 1.0) const;

  /// Mesh ``points`` at @p time.
  bool GetBasePoints(std::vector<freeusd::gf::Vec3f>* out, double time = 1.0) const;

  /// Read base points and apply morphs at @p time.
  bool EvaluatePoints(std::vector<freeusd::gf::Vec3f>* out, double time = 1.0) const;

  /// Tokens, target paths, and weights together (skips targets with empty paths).
  std::vector<MorphTargetBinding> ResolveBindings(const std::shared_ptr<const freeusd::usd::Stage>& stage,
                                                  double time = 1.0) const;

  /// Load ``BlendShape`` payloads for bound targets.
  std::vector<BlendShape> LoadBlendShapes(const std::shared_ptr<const freeusd::usd::Stage>& stage,
                                          double time = 1.0) const;

  /// ``point' = point + sum(weight_i * offset_i)`` (CPU); uses bindings at @p time.
  bool ApplyToPoints(const std::shared_ptr<const freeusd::usd::Stage>& stage,
                     const std::vector<freeusd::gf::Vec3f>& base_points, std::vector<freeusd::gf::Vec3f>* out,
                     double time = 1.0) const;
};

/// Apply morph deltas: dense or sparse via ``pointIndices``; @p weights aligns with @p blend_shapes.
FREEUSD_API bool ApplyMorphTargetsToPoints(const std::vector<freeusd::gf::Vec3f>& base_points,
                                           const std::vector<BlendShape>& blend_shapes,
                                           const std::vector<float>& weights, std::vector<freeusd::gf::Vec3f>* out,
                                           double time = 1.0);

}  // namespace freeusd::usdSkel
