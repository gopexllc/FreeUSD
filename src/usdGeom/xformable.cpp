#include "freeusd/usdGeom/xformable.hpp"

#include <cctype>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "freeusd/gf/quatd.hpp"
#include "freeusd/gf/quatf.hpp"
#include "freeusd/gf/vec3d.hpp"
#include "freeusd/gf/vec3f.hpp"
#include "freeusd/tf/token.hpp"
#include "freeusd/vt/value.hpp"

namespace freeusd::usdGeom {
namespace {

std::string trim(std::string s) {
  while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front()))) {
    s.erase(s.begin());
  }
  while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back()))) {
    s.pop_back();
  }
  if (s.size() >= 2 && ((s.front() == '"' && s.back() == '"') || (s.front() == '\'' && s.back() == '\''))) {
    s = s.substr(1, s.size() - 2);
  }
  return s;
}

std::vector<std::string> parse_xform_op_order(const freeusd::usd::Prim& prim, double time) {
  std::vector<std::string> out;
  const freeusd::tf::Token kOrder("xformOpOrder");
  if (!prim.HasAttribute(kOrder)) {
    return out;
  }
  const freeusd::vt::Value v = prim.GetAttribute(kOrder, time);
  std::vector<freeusd::tf::Token> toks;
  if (v.GetTokenArray(&toks)) {
    out.reserve(toks.size());
    for (const freeusd::tf::Token& t : toks) {
      if (!t.IsEmpty()) {
        out.push_back(t.GetText());
      }
    }
    return out;
  }
  std::string s;
  if (v.GetString(&s) && !s.empty()) {
    std::stringstream ss(s);
    std::string part;
    while (std::getline(ss, part, ',')) {
      const std::string t = trim(part);
      if (!t.empty()) {
        out.push_back(t);
      }
    }
    return out;
  }
  freeusd::tf::Token one;
  if (v.GetToken(&one) && !one.IsEmpty()) {
    out.push_back(one.GetText());
  }
  return out;
}

/// Public USD marker: ops at indices ``0 .. last`` inclusive are ignored for local xform when ``last`` is the last
/// ``!resetXformStack!`` entry (clean-room behavior per ``UsdGeomXformable`` docs).
static constexpr char kResetXformStackToken[] = "!resetXformStack!";

std::vector<std::string> xform_op_order_for_local_compose(const std::vector<std::string>& in) {
  std::vector<std::string> order = in;
  if (order.empty()) {
    return order;
  }
  int last_reset = -1;
  for (int i = static_cast<int>(order.size()) - 1; i >= 0; --i) {
    if (order[static_cast<std::size_t>(i)] == kResetXformStackToken) {
      last_reset = i;
      break;
    }
  }
  if (last_reset < 0) {
    return order;
  }
  order.erase(order.begin(), order.begin() + static_cast<std::size_t>(last_reset) + 1);
  return order;
}

bool prim_resets_xform_stack(const freeusd::usd::Prim& prim, double time) {
  const std::vector<std::string> raw = parse_xform_op_order(prim, time);
  for (const std::string& s : raw) {
    if (s == kResetXformStackToken) {
      return true;
    }
  }
  return false;
}

/// ``UsdGeomXformOp`` translate-family ops that use ``double3`` / ``vec3d`` (``GetOpTransform`` is ``Translate``).
bool is_xform_translate_vec3_op(const std::string& op) {
  static constexpr char kBase[] = "xformOp:translate";
  constexpr std::size_t kBaseLen = sizeof(kBase) - 1;
  if (op == kBase) {
    return true;
  }
  if (op.size() <= kBaseLen + 1 || op.compare(0, kBaseLen, kBase) != 0 || op[kBaseLen] != ':') {
    return false;
  }
  const std::string_view suf(op.data() + kBaseLen + 1, op.size() - kBaseLen - 1);
  return suf == "pivot" || suf == "pivotInverse";
}

bool read_angle_degrees(const freeusd::vt::Value& v, double* out) {
  if (v.GetDouble(out)) {
    return true;
  }
  float f = 0.0F;
  if (v.GetFloat(&f)) {
    *out = static_cast<double>(f);
    return true;
  }
  return false;
}

freeusd::gf::Matrix4d matrix_for_op(const freeusd::usd::Prim& prim, const std::string& op_token, double time) {
  using freeusd::gf::Matrix4d;
  using freeusd::tf::Token;

  static constexpr char kInvertPrefix[] = "!invert!";
  constexpr std::size_t kInvertLen = sizeof(kInvertPrefix) - 1;

  bool inverted = false;
  std::string op = op_token;
  if (op.size() >= kInvertLen && op.compare(0, kInvertLen, kInvertPrefix) == 0) {
    inverted = true;
    op = trim(op.substr(kInvertLen));
  }

  const freeusd::vt::Value v = prim.GetAttribute(Token{op.c_str()}, time);
  freeusd::gf::Vec3d vec{};
  bool have_vec3 = v.GetVec3d(&vec);
  if (!have_vec3) {
    freeusd::gf::Vec3f vf{};
    if (v.GetVec3f(&vf)) {
      vec.set(static_cast<double>(vf.x()), static_cast<double>(vf.y()), static_cast<double>(vf.z()));
      have_vec3 = true;
    }
  }
  Matrix4d forward = Matrix4d::Identity();
  if (have_vec3) {
    if (is_xform_translate_vec3_op(op)) {
      forward = Matrix4d::Translate(vec);
    } else if (op == "xformOp:scale") {
      forward = Matrix4d::Scale(vec.x(), vec.y(), vec.z());
    } else if (op == "xformOp:rotateXYZ") {
      forward = Matrix4d::RotateDegreesXYZ(vec.x(), vec.y(), vec.z());
    } else if (op == "xformOp:rotateXZY") {
      forward = Matrix4d::RotateDegreesXZY(vec.x(), vec.y(), vec.z());
    } else if (op == "xformOp:rotateYXZ") {
      forward = Matrix4d::RotateDegreesYXZ(vec.x(), vec.y(), vec.z());
    } else if (op == "xformOp:rotateYZX") {
      forward = Matrix4d::RotateDegreesYZX(vec.x(), vec.y(), vec.z());
    } else if (op == "xformOp:rotateZXY") {
      forward = Matrix4d::RotateDegreesZXY(vec.x(), vec.y(), vec.z());
    } else if (op == "xformOp:rotateZYX") {
      forward = Matrix4d::RotateDegreesZYX(vec.x(), vec.y(), vec.z());
    } else if (op == "xformOp:shear") {
      forward = Matrix4d::Shear(vec.x(), vec.y(), vec.z());
    }
  } else {
    freeusd::gf::Quatd quat{};
    if (op == "xformOp:orient") {
      if (v.GetQuatd(&quat)) {
        forward = Matrix4d::FromUnitQuaternion(quat);
      } else {
        freeusd::gf::Quatf qf{};
        if (v.GetQuatf(&qf)) {
          quat = freeusd::gf::Quatd{static_cast<double>(qf.real), static_cast<double>(qf.i), static_cast<double>(qf.j),
                                    static_cast<double>(qf.k)};
          forward = Matrix4d::FromUnitQuaternion(quat);
        }
      }
    } else {
      freeusd::gf::Matrix4d xf{};
      if (op == "xformOp:transform" && v.GetMatrix4d(&xf)) {
        forward = xf;
      } else {
        double ang = 0.0;
        if (read_angle_degrees(v, &ang)) {
          if (op == "xformOp:rotateX") {
            forward = Matrix4d::RotateDegreesX(ang);
          } else if (op == "xformOp:rotateY") {
            forward = Matrix4d::RotateDegreesY(ang);
          } else if (op == "xformOp:rotateZ") {
            forward = Matrix4d::RotateDegreesZ(ang);
          }
        }
      }
    }
  }

  if (!inverted) {
    return forward;
  }
  Matrix4d inv{};
  if (!forward.GetInverse(&inv)) {
    return Matrix4d::Identity();
  }
  return inv;
}

void push_if_attr(const freeusd::usd::Prim& prim, std::vector<std::string>& out, const char* attr_name) {
  if (prim.HasAttribute(freeusd::tf::Token(attr_name))) {
    out.emplace_back(attr_name);
  }
}

freeusd::gf::Matrix4d compose_local_from_prim(const freeusd::usd::Prim& prim, double time) {
  const std::vector<std::string> raw_order = parse_xform_op_order(prim, time);
  std::vector<std::string> order;
  if (raw_order.empty()) {
    push_if_attr(prim, order, "xformOp:translate");
    push_if_attr(prim, order, "xformOp:translate:pivot");
    static const char* kEulerPacked[] = {"xformOp:rotateXYZ", "xformOp:rotateXZY", "xformOp:rotateYXZ",
                                         "xformOp:rotateYZX", "xformOp:rotateZXY", "xformOp:rotateZYX"};
    for (const char* n : kEulerPacked) {
      push_if_attr(prim, order, n);
    }
    push_if_attr(prim, order, "xformOp:rotateX");
    push_if_attr(prim, order, "xformOp:rotateY");
    push_if_attr(prim, order, "xformOp:rotateZ");
    push_if_attr(prim, order, "xformOp:orient");
    push_if_attr(prim, order, "xformOp:transform");
    push_if_attr(prim, order, "xformOp:shear");
    push_if_attr(prim, order, "xformOp:translate:pivotInverse");
    push_if_attr(prim, order, "xformOp:scale");
  } else {
    order = xform_op_order_for_local_compose(raw_order);
  }
  freeusd::gf::Matrix4d m = freeusd::gf::Matrix4d::Identity();
  for (const std::string& op : order) {
    const freeusd::gf::Matrix4d part = matrix_for_op(prim, op, time);
    m = freeusd::gf::Matrix4d::Multiply(part, m);
  }
  return m;
}

}  // namespace

freeusd::gf::Matrix4d Xformable::ComputeLocalTransform(double time) const {
  if (!prim.IsValid()) {
    return freeusd::gf::Matrix4d::Identity();
  }
  return compose_local_from_prim(prim, time);
}

freeusd::gf::Matrix4d Xformable::ComputeLocalToWorldTransform(double time) const {
  if (!prim.IsValid()) {
    return freeusd::gf::Matrix4d::Identity();
  }
  freeusd::gf::Matrix4d m = freeusd::gf::Matrix4d::Identity();
  for (freeusd::usd::Prim p = prim; p.IsValid(); p = p.GetParent()) {
    const freeusd::gf::Matrix4d local = compose_local_from_prim(p, time);
    m = freeusd::gf::Matrix4d::Multiply(local, m);
    if (prim_resets_xform_stack(p, time)) {
      break;
    }
  }
  return m;
}

freeusd::gf::Matrix4d Xformable::ComputeParentToWorldTransform(double time) const {
  if (!prim.IsValid()) {
    return freeusd::gf::Matrix4d::Identity();
  }
  const freeusd::usd::Prim par = prim.GetParent();
  if (!par.IsValid()) {
    return freeusd::gf::Matrix4d::Identity();
  }
  return Xformable(par).ComputeLocalToWorldTransform(time);
}

}  // namespace freeusd::usdGeom
