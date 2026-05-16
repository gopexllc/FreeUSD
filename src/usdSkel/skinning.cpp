#include "freeusd/usdSkel/skinning.hpp"

#include <cstddef>
#include <vector>

#include "freeusd/gf/quatd.hpp"
#include "freeusd/gf/vec3d.hpp"
#include "freeusd/usd/stage.hpp"
#include "freeusd/usdSkel/gltfMapping.hpp"
#include "freeusd/usdSkel/skelAnimation.hpp"
#include "freeusd/usdSkel/skeleton.hpp"

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

freeusd::gf::Matrix4d local_from_joint_transform(const JointTransform& jt) {
  freeusd::gf::Matrix4d local = freeusd::gf::Matrix4d::Identity();
  if (jt.has_translation) {
    local = freeusd::gf::Matrix4d::Multiply(
        local, freeusd::gf::Matrix4d::Translate(static_cast<double>(jt.translation.x()),
                                                  static_cast<double>(jt.translation.y()),
                                                  static_cast<double>(jt.translation.z())));
  }
  if (jt.has_rotation) {
    const freeusd::gf::Quatd q(jt.rotation.real, jt.rotation.i, jt.rotation.j, jt.rotation.k);
    local = freeusd::gf::Matrix4d::Multiply(local, freeusd::gf::Matrix4d::FromUnitQuaternion(q));
  }
  if (jt.has_scale) {
    local = freeusd::gf::Matrix4d::Multiply(
        local, freeusd::gf::Matrix4d::Scale(static_cast<double>(jt.scale.x()), static_cast<double>(jt.scale.y()),
                                            static_cast<double>(jt.scale.z())));
  }
  return local;
}

}  // namespace

bool BuildJointWorldMatricesFromAnimation(const Skeleton& skeleton, const SkelAnimation& animation, double time,
                                          std::vector<freeusd::gf::Matrix4d>* out_world) {
  if (!out_world || !skeleton || !animation) {
    return false;
  }
  const std::vector<std::string> joints = skeleton.GetJointNames();
  if (joints.empty()) {
    return false;
  }
  const std::shared_ptr<const freeusd::usd::Stage> stage = skeleton.GetPrim().GetStage();
  if (!stage) {
    return false;
  }
  const freeusd::sdf::Path anim_path = animation.GetPrim().GetPath();

  std::vector<freeusd::gf::Matrix4d> locals;
  locals.reserve(joints.size());
  for (std::size_t i = 0; i < joints.size(); ++i) {
    JointTransform sample{};
    if (SkelAnimation::SampleJointTransform(stage, anim_path, i, time, &sample)) {
      locals.push_back(local_from_joint_transform(sample));
    } else {
      locals.push_back(freeusd::gf::Matrix4d::Identity());
    }
  }
  *out_world = AccumulateWorldTransforms(skeleton.GetJointParentIndices(), locals);
  return !out_world->empty();
}

bool ComputeSkinningMatrices(const std::vector<freeusd::gf::Matrix4d>& joint_world_matrices,
                             const std::vector<freeusd::gf::Matrix4d>& inverse_bind_matrices,
                             std::vector<freeusd::gf::Matrix4d>* out_palette) {
  if (!out_palette || joint_world_matrices.empty() ||
      joint_world_matrices.size() != inverse_bind_matrices.size()) {
    return false;
  }
  out_palette->resize(joint_world_matrices.size());
  for (std::size_t i = 0; i < joint_world_matrices.size(); ++i) {
    (*out_palette)[i] =
        freeusd::gf::Matrix4d::Multiply(joint_world_matrices[i], inverse_bind_matrices[i]);
  }
  return true;
}

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

  std::vector<freeusd::gf::Matrix4d> palette;
  if (!ComputeSkinningMatrices(joint_world_matrices, inverse_bind_matrices, &palette)) {
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
      if (joint < 0 || static_cast<std::size_t>(joint) >= palette.size()) {
        continue;
      }
      const freeusd::gf::Vec3d influenced =
          transform_point(palette[static_cast<std::size_t>(joint)], bind_point);
      deformed.set(deformed.x() + static_cast<double>(weight) * influenced.x(),
                   deformed.y() + static_cast<double>(weight) * influenced.y(),
                   deformed.z() + static_cast<double>(weight) * influenced.z());
    }
    (*out)[vtx] = to_vec3f(deformed);
  }
  return true;
}

}  // namespace freeusd::usdSkel
