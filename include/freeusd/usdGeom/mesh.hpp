#pragma once

#include <vector>

#include "freeusd/gf/vec3f.hpp"
#include "freeusd/usd/prim.hpp"

namespace freeusd::usdGeom {

/// ``UsdGeomMesh``-shaped helper for a narrow executable subset: ``points`` and ``faceVertexCounts``.
struct FREEUSD_API Mesh {
  freeusd::usd::Prim prim;

  Mesh() = default;
  explicit Mesh(freeusd::usd::Prim p) : prim(std::move(p)) {}

  explicit operator bool() const noexcept { return prim.IsValid(); }
  const freeusd::usd::Prim& GetPrim() const noexcept { return prim; }

  /// Composed ``point3f[]`` / ``points`` at @p time (empty if missing or unsupported).
  std::vector<freeusd::gf::Vec3f> GetPoints(double time = 1.0) const;

  /// Composed ``int[]`` / ``faceVertexCounts`` at @p time (empty if missing or unsupported).
  std::vector<int> GetFaceVertexCounts(double time = 1.0) const;
};

}  // namespace freeusd::usdGeom
