#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "freeusd/export.hpp"
#include "freeusd/gf/matrix4d.hpp"

namespace freeusd::usdSkel {

/// Build parent joint indices from slash-separated joint path tokens (glTF skin joint order).
FREEUSD_API std::vector<int> BuildJointParentIndices(const std::vector<std::string>& joint_names);

/// Compose world-space joint matrices from parent-relative locals (row-vector ``v * M``).
FREEUSD_API std::vector<freeusd::gf::Matrix4d> AccumulateWorldTransforms(
    const std::vector<int>& parent_indices, const std::vector<freeusd::gf::Matrix4d>& local_transforms);

/// glTF-oriented alias for bind-pose world matrices (same as ``AccumulateWorldTransforms``).
inline std::vector<freeusd::gf::Matrix4d> ComputeWorldBindMatrices(
    const std::vector<int>& parent_indices, const std::vector<freeusd::gf::Matrix4d>& local_bind_transforms) {
  return AccumulateWorldTransforms(parent_indices, local_bind_transforms);
}

}  // namespace freeusd::usdSkel
