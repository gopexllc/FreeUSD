#include "freeusd/usdSkel/blendShape.hpp"

#include <vector>

#include "freeusd/gf/vec3d.hpp"
#include "freeusd/usd/stage.hpp"
#include "freeusd/usdSkel/tokens.hpp"
#include "freeusd/vt/value.hpp"

namespace freeusd::usdSkel {
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

BlendShape BlendShape::ReadFromPrim(const std::shared_ptr<const freeusd::usd::Stage>& stage,
                                    const freeusd::sdf::Path& path) {
  if (!stage) {
    return {};
  }
  return BlendShape(stage->GetPrimAtPath(path));
}

bool BlendShape::GetOffsets(std::vector<freeusd::gf::Vec3f>* out, double time) const {
  if (!out || !prim.IsValid()) {
    return false;
  }
  return read_vec3f_array(prim.GetAttribute(tokens::offsets(), time), out);
}

bool BlendShape::GetPointIndices(std::vector<int>* out, double time) const {
  if (!out || !prim.IsValid()) {
    return false;
  }
  out->clear();
  (void)read_int_array(prim.GetAttribute(tokens::pointIndices(), time), out);
  return true;
}

}  // namespace freeusd::usdSkel
