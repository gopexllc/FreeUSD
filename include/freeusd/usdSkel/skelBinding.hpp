#pragma once

#include <cstddef>
#include <optional>
#include <vector>

#include "freeusd/export.hpp"
#include "freeusd/sdf/path.hpp"
#include "freeusd/usd/prim.hpp"

namespace freeusd::usdSkel {

/// Mesh (or other geom) skinning binding: ``skel:skeleton`` plus joint primvars.
struct FREEUSD_API SkelBinding {
  freeusd::usd::Prim geom_prim;
  freeusd::sdf::Path skeleton_path{};

  SkelBinding() = default;
  explicit SkelBinding(freeusd::usd::Prim geom) : geom_prim(std::move(geom)) {}

  explicit operator bool() const noexcept { return geom_prim.IsValid() && !skeleton_path.IsEmpty(); }

  /// Resolve ``rel skel:skeleton`` targets, else ``token skel:skeleton`` on @p geom.
  static std::optional<freeusd::sdf::Path> ResolveSkeletonPath(const freeusd::usd::Prim& geom);

  /// ``primvars:skel:jointIndices`` (``int[]``) at @p time.
  static bool ReadJointIndices(const freeusd::usd::Prim& geom, std::vector<int>* out, double time = 1.0);

  /// ``primvars:skel:jointWeights`` (``float[]``) at @p time.
  static bool ReadJointWeights(const freeusd::usd::Prim& geom, std::vector<float>* out, double time = 1.0);

  /// True when index and weight arrays match; optional @p influences_per_point must divide length when > 0.
  static bool ValidateInfluenceCounts(const std::vector<int>& indices, const std::vector<float>& weights,
                                      std::size_t influences_per_point = 4);

  /// Populate binding from a geom prim (skeleton path plus joint primvars when present).
  static SkelBinding ReadFromGeomPrim(const freeusd::usd::Prim& geom, double time = 1.0);
};

}  // namespace freeusd::usdSkel
