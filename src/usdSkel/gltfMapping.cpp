#include "freeusd/usdSkel/gltfMapping.hpp"

#include <cstddef>
#include <vector>

#include "freeusd/gf/matrix4d.hpp"

namespace freeusd::usdSkel {

std::vector<int> BuildJointParentIndices(const std::vector<std::string>& joint_names) {
  std::vector<int> parents(joint_names.size(), -1);
  for (std::size_t i = 0; i < joint_names.size(); ++i) {
    const std::string& name = joint_names[i];
    const std::size_t slash = name.rfind('/');
    if (slash == std::string::npos || slash == 0) {
      continue;
    }
    const std::string parent_path = name.substr(0, slash);
    for (std::size_t j = 0; j < joint_names.size(); ++j) {
      if (joint_names[j] == parent_path) {
        parents[i] = static_cast<int>(j);
        break;
      }
    }
  }
  return parents;
}

std::vector<freeusd::gf::Matrix4d> AccumulateWorldTransforms(
    const std::vector<int>& parent_indices, const std::vector<freeusd::gf::Matrix4d>& local_transforms) {
  const std::size_t n = local_transforms.size();
  std::vector<freeusd::gf::Matrix4d> world(n, freeusd::gf::Matrix4d::Identity());
  for (std::size_t i = 0; i < n; ++i) {
    const int parent = (i < parent_indices.size()) ? parent_indices[i] : -1;
    if (parent < 0) {
      world[i] = local_transforms[i];
    } else if (static_cast<std::size_t>(parent) < n) {
      world[i] = freeusd::gf::Matrix4d::Multiply(local_transforms[i], world[static_cast<std::size_t>(parent)]);
    } else {
      world[i] = local_transforms[i];
    }
  }
  return world;
}

}  // namespace freeusd::usdSkel
