#include "freeusd/usdSkel/skeleton.hpp"

#include <string>
#include <vector>

#include "freeusd/usd/stage.hpp"
#include "freeusd/usdSkel/gltfMapping.hpp"
#include "freeusd/usdSkel/tokens.hpp"
#include "freeusd/vt/value.hpp"

namespace freeusd::usdSkel {
namespace {

std::vector<std::string> read_joint_names(const freeusd::usd::Prim& prim) {
  std::vector<std::string> names;
  if (!prim.IsValid()) {
    return names;
  }
  const freeusd::vt::Value v = prim.GetAttribute(tokens::joints(), 1.0);
  std::vector<freeusd::tf::Token> toks;
  if (!v.GetTokenArray(&toks)) {
    return names;
  }
  names.reserve(toks.size());
  for (const freeusd::tf::Token& t : toks) {
    if (!t.IsEmpty()) {
      names.push_back(t.GetText());
    }
  }
  return names;
}

bool read_matrix4d_array_attr(const freeusd::usd::Prim& prim, const freeusd::tf::Token& name, double time,
                              std::vector<freeusd::gf::Matrix4d>* out) {
  if (!out || !prim.IsValid()) {
    return false;
  }
  const freeusd::vt::Value v = prim.GetAttribute(name, time);
  return v.GetMatrix4dArray(out) && !out->empty();
}

bool read_per_joint_matrix_attrs(const freeusd::usd::Prim& prim, const std::string& prefix, std::size_t joint_count,
                                 double time, std::vector<freeusd::gf::Matrix4d>* out) {
  if (!out || !prim.IsValid() || joint_count == 0) {
    return false;
  }
  out->clear();
  out->reserve(joint_count);
  for (std::size_t i = 0; i < joint_count; ++i) {
    const freeusd::tf::Token attr_name(prefix + std::to_string(i));
    const freeusd::vt::Value v = prim.GetAttribute(attr_name, time);
    freeusd::gf::Matrix4d m{};
    if (!v.GetMatrix4d(&m)) {
      return false;
    }
    out->push_back(m);
  }
  return out->size() == joint_count;
}

bool fill_identity_binds(std::size_t joint_count, std::vector<freeusd::gf::Matrix4d>* out) {
  if (!out) {
    return false;
  }
  out->assign(joint_count, freeusd::gf::Matrix4d::Identity());
  return joint_count > 0;
}

}  // namespace

Skeleton Skeleton::ReadFromPrim(const std::shared_ptr<const freeusd::usd::Stage>& stage,
                                const freeusd::sdf::Path& path) {
  if (!stage) {
    return {};
  }
  return Skeleton(stage->GetPrimAtPath(path));
}

std::vector<std::string> Skeleton::GetJointNames() const { return read_joint_names(prim); }

std::vector<int> Skeleton::GetJointParentIndices() const {
  return BuildJointParentIndices(GetJointNames());
}

bool Skeleton::GetBindTransforms(std::vector<freeusd::gf::Matrix4d>* out, double time) const {
  if (!out || !prim.IsValid()) {
    return false;
  }
  if (read_matrix4d_array_attr(prim, tokens::bindTransforms(), time, out)) {
    return true;
  }
  const std::vector<std::string> joints = GetJointNames();
  if (joints.empty()) {
    return false;
  }
  if (read_per_joint_matrix_attrs(prim, "bindTransform_", joints.size(), time, out)) {
    return true;
  }
  return fill_identity_binds(joints.size(), out);
}

bool Skeleton::GetRestTransforms(std::vector<freeusd::gf::Matrix4d>* out, double time) const {
  if (!out || !prim.IsValid()) {
    return false;
  }
  if (read_matrix4d_array_attr(prim, tokens::restTransforms(), time, out)) {
    return true;
  }
  const std::vector<std::string> joints = GetJointNames();
  if (joints.empty()) {
    return false;
  }
  if (read_per_joint_matrix_attrs(prim, "restTransform_", joints.size(), time, out)) {
    return true;
  }
  return fill_identity_binds(joints.size(), out);
}

std::vector<freeusd::gf::Matrix4d> Skeleton::ComputeWorldBindMatrices(double time) const {
  std::vector<freeusd::gf::Matrix4d> locals;
  if (!GetBindTransforms(&locals, time)) {
    return {};
  }
  return AccumulateWorldTransforms(GetJointParentIndices(), locals);
}

}  // namespace freeusd::usdSkel
