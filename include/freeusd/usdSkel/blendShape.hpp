#pragma once

#include <cstddef>
#include <vector>

#include "freeusd/export.hpp"
#include "freeusd/gf/vec3f.hpp"
#include "freeusd/sdf/path.hpp"
#include "freeusd/usd/prim.hpp"

namespace freeusd::usd {
class Stage;
}

namespace freeusd::usdSkel {

/// ``UsdSkelBlendShape``-shaped helper: per-point position deltas (glTF morph target POSITION).
struct FREEUSD_API BlendShape {
  freeusd::usd::Prim prim;

  BlendShape() = default;
  explicit BlendShape(freeusd::usd::Prim p) : prim(std::move(p)) {}

  static BlendShape ReadFromPrim(const std::shared_ptr<const freeusd::usd::Stage>& stage,
                                 const freeusd::sdf::Path& path);

  explicit operator bool() const noexcept { return prim.IsValid(); }

  /// ``offsets`` (``point3f[]`` / ``float3[]``) at @p time.
  bool GetOffsets(std::vector<freeusd::gf::Vec3f>* out, double time = 1.0) const;

  /// Optional sparse mapping; empty means dense correspondence with mesh points.
  bool GetPointIndices(std::vector<int>* out, double time = 1.0) const;
};

}  // namespace freeusd::usdSkel
