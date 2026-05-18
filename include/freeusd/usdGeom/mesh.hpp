#pragma once

#include <vector>

#include "freeusd/gf/vec3f.hpp"
#include "freeusd/usd/prim.hpp"

namespace freeusd::usdGeom {

/// 2D texture coordinate (``texCoord2f`` / ``primvars:st``).
struct FREEUSD_API TexCoord2f {
  float s = 0.0f;
  float t = 0.0f;
};

/// ``UsdGeomMesh``-shaped helper for a narrow executable subset of mesh attributes.
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

  /// Composed ``int[]`` / ``faceVertexIndices`` at @p time (empty if missing or unsupported).
  std::vector<int> GetFaceVertexIndices(double time = 1.0) const;

  /// Composed ``primvars:displayColor`` at @p time (empty if missing or unsupported).
  std::vector<freeusd::gf::Vec3f> GetDisplayColor(double time = 1.0) const;

  /// Composed ``normal3f[]`` / ``normals`` at @p time (empty if missing or unsupported).
  std::vector<freeusd::gf::Vec3f> GetNormals(double time = 1.0) const;

  /// Composed ``texCoord2f[]`` / ``primvars:st`` at @p time (empty if missing or unsupported).
  std::vector<TexCoord2f> GetPrimvarsSt(double time = 1.0) const;

  /// Composed ``primvars:displayOpacity`` scalar or first element of a float array at @p time.
  bool GetDisplayOpacity(float* out, double time = 1.0) const;
};

}  // namespace freeusd::usdGeom
