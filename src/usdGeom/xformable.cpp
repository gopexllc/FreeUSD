#include "freeusd/usdGeom/xformable.hpp"

#include <cctype>
#include <sstream>
#include <string>
#include <vector>

#include "freeusd/gf/vec3d.hpp"
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
  std::string s;
  if (!v.GetString(&s) || s.empty()) {
    return out;
  }
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

freeusd::gf::Matrix4d matrix_for_op(const freeusd::usd::Prim& prim, const std::string& op, double time) {
  using freeusd::tf::Token;
  const freeusd::vt::Value v = prim.GetAttribute(Token{op.c_str()}, time);
  freeusd::gf::Vec3d vec{};
  if (v.GetVec3d(&vec)) {
    if (op == "xformOp:translate") {
      return freeusd::gf::Matrix4d::Translate(vec);
    }
    if (op == "xformOp:scale") {
      return freeusd::gf::Matrix4d::Scale(vec.x(), vec.y(), vec.z());
    }
  }
  if (op.rfind("xformOp:rotate", 0) == 0) {
    return freeusd::gf::Matrix4d::Identity();
  }
  return freeusd::gf::Matrix4d::Identity();
}

freeusd::gf::Matrix4d compose_local_from_prim(const freeusd::usd::Prim& prim, double time) {
  std::vector<std::string> order = parse_xform_op_order(prim, time);
  if (order.empty()) {
    if (prim.HasAttribute(freeusd::tf::Token("xformOp:translate"))) {
      order.push_back("xformOp:translate");
    }
    if (prim.HasAttribute(freeusd::tf::Token("xformOp:scale"))) {
      order.push_back("xformOp:scale");
    }
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
  }
  return m;
}

}  // namespace freeusd::usdGeom
