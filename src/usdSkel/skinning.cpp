#include "freeusd/usdSkel/skinning.hpp"

#include <cstddef>
#include <vector>

#include "freeusd/gf/vec3d.hpp"

namespace freeusd::usdSkel {
namespace {

freeusd::gf::Vec3d transform_point(const freeusd::gf::Matrix4d& m, const freeusd::gf::Vec3d& p) {
  const double* v = m.data();
  const double x = p.x() * v[0] + p.y() * v[4] + p.z() * v[8] + v[12];
  const double y = p.x() * v[1] + p.y() * v[5] + p.z() * v[9] + v[13];
  const double z = p.x() * v[2] + p.y() * v[6] + p.z() * v[10] + v[14];
  const double w = p.x() * v[3] + p.y() * v[7] + p.z() * v[11] + v[15];
  freeusd::gf::Vec3d out{};
  if (w != 0.0 && w != 1.0) {
    out.set(x / w, y / w, z / w);
  } else {
    out.set(x, y, z);
  }
  return out;
}

freeusd::gf::Vec3f to_vec3f(const freeusd::gf::Vec3d& p) {
  freeusd::gf::Vec3f out{};
  out.set(static_cast<float>(p.x()), static_cast<float>(p.y()), static_cast<float>(p.z()));
  return out;
}

}  // namespace

bool DeformPointsWithSkeleton(const std::vector<freeusd::gf::Vec3f>& points, const std::vector<int>& joint_indices,
                              const std::vector<float>& joint_weights, std::size_t influences_per_point,
                              const std::vector<freeusd::gf::Matrix4d>& joint_world_matrices,
                              const std::vector<freeusd::gf::Matrix4d>& inverse_bind_matrices,
                              const freeusd::gf::Matrix4d* geom_bind, std::vector<freeusd::gf::Vec3f>* out) {
  if (!out || points.empty() || influences_per_point == 0) {
    return false;
  }
  if (joint_indices.size() != joint_weights.size()) {
    return false;
  }
  if (joint_indices.size() % influences_per_point != 0) {
    return false;
  }
  const std::size_t point_count = points.size();
  const std::size_t vertex_count = joint_indices.size() / influences_per_point;
  if (vertex_count != point_count) {
    return false;
  }

  out->resize(point_count);
  for (std::size_t vtx = 0; vtx < point_count; ++vtx) {
    freeusd::gf::Vec3d bind_point{static_cast<double>(points[vtx].x()), static_cast<double>(points[vtx].y()),
                                  static_cast<double>(points[vtx].z())};
    if (geom_bind) {
      bind_point = transform_point(*geom_bind, bind_point);
    }

    freeusd::gf::Vec3d deformed{};
    deformed.set(0.0, 0.0, 0.0);
    const std::size_t base = vtx * influences_per_point;
    for (std::size_t inf = 0; inf < influences_per_point; ++inf) {
      const float weight = joint_weights[base + inf];
      if (weight == 0.0F) {
        continue;
      }
      const int joint = joint_indices[base + inf];
      if (joint < 0 || static_cast<std::size_t>(joint) >= joint_world_matrices.size() ||
          static_cast<std::size_t>(joint) >= inverse_bind_matrices.size()) {
        continue;
      }
      const freeusd::gf::Matrix4d skin =
          freeusd::gf::Matrix4d::Multiply(inverse_bind_matrices[static_cast<std::size_t>(joint)],
                                          joint_world_matrices[static_cast<std::size_t>(joint)]);
      const freeusd::gf::Vec3d influenced = transform_point(skin, bind_point);
      deformed.set(deformed.x() + static_cast<double>(weight) * influenced.x(),
                   deformed.y() + static_cast<double>(weight) * influenced.y(),
                   deformed.z() + static_cast<double>(weight) * influenced.z());
    }
    (*out)[vtx] = to_vec3f(deformed);
  }
  return true;
}

}  // namespace freeusd::usdSkel
