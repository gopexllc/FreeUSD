#include "freeusd/usdGeom/mesh.hpp"

#include "freeusd/tf/token.hpp"
#include "freeusd/vt/value.hpp"

namespace freeusd::usdGeom {
namespace {

bool read_vec3f_array(const freeusd::vt::Value& v, std::vector<freeusd::gf::Vec3f>* out) {
  if (!out) {
    return false;
  }
  if (v.GetVec3fArray(out) && !out->empty()) {
    return true;
  }
  freeusd::gf::Vec3f single{};
  if (v.GetVec3f(&single)) {
    out->assign(1, single);
    return true;
  }
  freeusd::gf::Vec3d vd{};
  if (v.GetVec3d(&vd)) {
    out->assign(1, freeusd::gf::Vec3f{});
    (*out)[0].set(static_cast<float>(vd.x()), static_cast<float>(vd.y()), static_cast<float>(vd.z()));
    return true;
  }
  return false;
}

bool read_int_array(const freeusd::vt::Value& v, std::vector<int>* out) {
  if (!out) {
    return false;
  }
  std::vector<std::int32_t> raw;
  if (v.GetInt32Array(&raw)) {
    out->assign(raw.begin(), raw.end());
    return !out->empty();
  }
  std::int32_t i32{};
  std::int64_t i64{};
  if (v.GetInt32(&i32)) {
    out->assign(1, static_cast<int>(i32));
    return true;
  }
  if (v.GetInt64(&i64)) {
    out->assign(1, static_cast<int>(i64));
    return true;
  }
  return false;
}

}  // namespace

std::vector<freeusd::gf::Vec3f> Mesh::GetPoints(double time) const {
  std::vector<freeusd::gf::Vec3f> points;
  if (!prim.IsValid()) {
    return points;
  }
  const freeusd::vt::Value v = prim.GetAttribute(freeusd::tf::Token("points"), time);
  read_vec3f_array(v, &points);
  return points;
}

std::vector<int> Mesh::GetFaceVertexCounts(double time) const {
  std::vector<int> counts;
  if (!prim.IsValid()) {
    return counts;
  }
  const freeusd::vt::Value v = prim.GetAttribute(freeusd::tf::Token("faceVertexCounts"), time);
  read_int_array(v, &counts);
  return counts;
}

}  // namespace freeusd::usdGeom
