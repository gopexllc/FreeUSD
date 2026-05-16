#pragma once

#include <cstddef>
#include <vector>

#include "freeusd/export.hpp"
#include "freeusd/gf/matrix4d.hpp"
#include "freeusd/gf/vec3f.hpp"

namespace freeusd::usdSkel {

class SkelAnimation;
class Skeleton;

/// Joint palette for GPU skinning (glTF ``skin.jointMatrices`` family).
///
/// For each joint @c i: ``palette[i] = jointWorld[i] * inverseBind[i]`` with row vectors ``[x y z 1] * M``.
/// @p inverse_bind_matrices are skeleton ``bindTransforms`` (inverse bind / rest-to-joint).
FREEUSD_API bool ComputeSkinningMatrices(const std::vector<freeusd::gf::Matrix4d>& joint_world_matrices,
                                         const std::vector<freeusd::gf::Matrix4d>& inverse_bind_matrices,
                                         std::vector<freeusd::gf::Matrix4d>* out_palette);

/// Linear blend skinning (LBS) on CPU.
///
/// glTF mapping (per influence @c i):
///   @c skinMatrix[i] = jointWorld[i] * inverseBind[i]
///   @c point' = sum(weight[i] * (point * skinMatrix[i]))
///
/// Row-vector convention: ``[x y z 1] * M``. @p inverse_bind_matrices are skeleton ``bindTransforms``
/// (inverse bind / rest-to-joint). Optional @p geom_bind is applied first (``primvars:skel:geomBindTransform``).
FREEUSD_API bool DeformPointsWithSkeleton(const std::vector<freeusd::gf::Vec3f>& points,
                                          const std::vector<int>& joint_indices,
                                          const std::vector<float>& joint_weights,
                                          std::size_t influences_per_point,
                                          const std::vector<freeusd::gf::Matrix4d>& joint_world_matrices,
                                          const std::vector<freeusd::gf::Matrix4d>& inverse_bind_matrices,
                                          const freeusd::gf::Matrix4d* geom_bind,
                                          std::vector<freeusd::gf::Vec3f>* out);

/// Build per-joint world matrices from sampled TRS (uses skeleton joint order / parents).
FREEUSD_API bool BuildJointWorldMatricesFromAnimation(const Skeleton& skeleton, const SkelAnimation& animation,
                                                      double time, std::vector<freeusd::gf::Matrix4d>* out_world);

}  // namespace freeusd::usdSkel
