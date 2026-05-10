#pragma once

#include "freeusd/gf/matrix4d.hpp"
#include "freeusd/usd/prim.hpp"

namespace freeusd::usdGeom {

/// ``UsdGeomXformable``-shaped helper: reads a subset of ``xformOp`` attributes and walks the prim chain.
///
/// Supported: ``xformOp:translate`` and Maya-style pivot translates ``xformOp:translate:pivot`` /
/// ``xformOp:translate:pivotInverse`` (each ``vec3d`` / ``vec3f`` / ``Translate``); ``xformOp:scale`` as ``vec3d`` or ``vec3f``; packed Euler ``xformOp:rotateXYZ`` and
/// ``rotateXZY`` / ``rotateYXZ`` / ``rotateYZX`` / ``rotateZXY`` / ``rotateZYX`` (degrees, ``vec3d`` / ``vec3f``); axis
/// ``xformOp:rotateX`` / ``rotateY`` / ``rotateZ`` (degrees, ``double`` or ``float``); ``xformOp:shear`` as ``vec3d``
/// (upper-triangular shear via ``gf::Matrix4d::Shear``, ``vec3d`` / ``vec3f``); ``xformOp:orient`` as ``quatd`` or ``quatf``
/// (``(w, i, j, k)``); ``xformOp:transform`` as ``matrix4d`` (nested ``((row0), …)`` row-major ``gf::Matrix4d``). Ops
/// apply in ``xformOpOrder``
/// when that attribute is a ``token[]`` value, a single ``token``, a comma-separated ``string``, or (if absent) a
/// fixed fallback lists translate, ``translate:pivot``, any
/// authored packed Euler (in the order above), axis rotates, orient, transform, ``translate:pivotInverse``, then scale. Row-vector ``[x y z 1] * M`` matches
/// ``GfMatrix4d`` / ``UsdGeomXformOp::GetOpTransform`` for these ops. ``xformOpOrder`` entries may use the OpenUSD
/// ``!invert!`` prefix (e.g. ``!invert!xformOp:translate``); the attribute name stays ``xformOp:translate`` and the
/// composed matrix uses ``GetInverse`` on the forward op matrix (identity if singular). The marker
/// ``!resetXformStack!`` in ``xformOpOrder`` (public USD token) drops all ops through the **last** such marker for
/// **local** composition, and ``ComputeLocalToWorldTransform`` stops accumulating **ancestor** locals once it hits a
/// prim whose order contains that marker (so that prim does not inherit its parent's cumulative transform).
struct FREEUSD_API Xformable {
  freeusd::usd::Prim prim;

  Xformable() = default;
  explicit Xformable(freeusd::usd::Prim p) : prim(std::move(p)) {}

  explicit operator bool() const noexcept { return prim.IsValid(); }
  const freeusd::usd::Prim& GetPrim() const noexcept { return prim; }

  /// Local transform from authored ``xformOp``\ s at \p time (defaults to ``1.0``).
  freeusd::gf::Matrix4d ComputeLocalTransform(double time = 1.0) const;

  /// Concatenation of ancestor locals from root toward \p prim (same \p time).
  freeusd::gf::Matrix4d ComputeLocalToWorldTransform(double time = 1.0) const;

  /// World transform of this prim's parent (identity if there is no parent). Matches
  /// ``UsdGeomXformable::ComputeParentToWorldTransform``.
  freeusd::gf::Matrix4d ComputeParentToWorldTransform(double time = 1.0) const;
};

}  // namespace freeusd::usdGeom
