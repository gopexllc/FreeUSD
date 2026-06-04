#include "freeusd/usdGeom/mesh.hpp"

#include "freeusd/tf/token.hpp"
#include "freeusd/usdGeom/tokens.hpp"

#include <string>
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

bool read_texcoord2f_array(const freeusd::vt::Value& v, std::vector<TexCoord2f>* out) {
  if (!out) {
    return false;
  }
  std::vector<float> floats;
  if (v.GetFloatArray(&floats) && floats.size() >= 2u && (floats.size() % 2u) == 0u) {
    out->reserve(floats.size() / 2u);
    for (std::size_t i = 0; i + 1u < floats.size(); i += 2u) {
      out->push_back(TexCoord2f{floats[i], floats[i + 1u]});
    }
    return !out->empty();
  }
  std::vector<freeusd::gf::Vec3f> vec3s;
  if (read_vec3f_array(v, &vec3s)) {
    out->reserve(vec3s.size());
    for (const freeusd::gf::Vec3f& v3 : vec3s) {
      out->push_back(TexCoord2f{v3.x(), v3.y()});
    }
    return !out->empty();
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
  const freeusd::vt::Value v = prim.GetAttribute(tokens::points(), time);
  read_vec3f_array(v, &points);
  return points;
}

std::vector<int> Mesh::GetFaceVertexCounts(double time) const {
  std::vector<int> counts;
  if (!prim.IsValid()) {
    return counts;
  }
  const freeusd::vt::Value v = prim.GetAttribute(tokens::faceVertexCounts(), time);
  read_int_array(v, &counts);
  return counts;
}

std::vector<int> Mesh::GetFaceVertexIndices(double time) const {
  std::vector<int> indices;
  if (!prim.IsValid()) {
    return indices;
  }
  const freeusd::vt::Value v = prim.GetAttribute(tokens::faceVertexIndices(), time);
  read_int_array(v, &indices);
  return indices;
}

std::vector<freeusd::gf::Vec3f> Mesh::GetDisplayColor(double time) const {
  std::vector<freeusd::gf::Vec3f> colors;
  if (!prim.IsValid()) {
    return colors;
  }
  const freeusd::vt::Value v = prim.GetAttribute(tokens::primvars_displayColor(), time);
  read_vec3f_array(v, &colors);
  return colors;
}

std::vector<freeusd::gf::Vec3f> Mesh::GetNormals(double time) const {
  std::vector<freeusd::gf::Vec3f> normals;
  if (!prim.IsValid()) {
    return normals;
  }
  const freeusd::vt::Value v = prim.GetAttribute(tokens::normals(), time);
  read_vec3f_array(v, &normals);
  return normals;
}

std::vector<TexCoord2f> Mesh::GetPrimvarsSt(double time) const {
  std::vector<TexCoord2f> st;
  if (!prim.IsValid()) {
    return st;
  }
  static const freeusd::tf::Token kPrimvarsSt("primvars:st");
  const freeusd::vt::Value v = prim.GetAttribute(kPrimvarsSt, time);
  read_texcoord2f_array(v, &st);
  return st;
}

bool Mesh::GetDisplayOpacity(float* out, double time) const {
  if (!out || !prim.IsValid()) {
    return false;
  }
  const freeusd::vt::Value v = prim.GetAttribute(tokens::primvars_displayOpacity(), time);
  if (v.GetFloat(out)) {
    return true;
  }
  double d = 0.0;
  if (v.GetDouble(&d)) {
    *out = static_cast<float>(d);
    return true;
  }
  std::vector<float> floats;
  if (v.GetFloatArray(&floats) && !floats.empty()) {
    *out = floats.front();
    return true;
  }
  return false;
}

bool Mesh::GetExtent(freeusd::gf::Vec3f* min_out, freeusd::gf::Vec3f* max_out, double time) const {
  if (!min_out || !max_out || !prim.IsValid()) {
    return false;
  }
  const freeusd::vt::Value v = prim.GetAttribute(tokens::extent(), time);
  std::vector<freeusd::gf::Vec3f> corners;
  if (!read_vec3f_array(v, &corners) || corners.size() < 2u) {
    return false;
  }
  *min_out = corners[0];
  *max_out = corners[1];
  return true;
}

bool Mesh::GetSubdivisionScheme(std::string* out, double time) const {
  if (!out || !prim.IsValid()) {
    return false;
  }
  const freeusd::vt::Value v = prim.GetAttribute(tokens::subdivisionScheme(), time);
  freeusd::tf::Token token;
  if (v.GetToken(&token)) {
    *out = token.GetText();
    return true;
  }
  return v.GetString(out);
}

}  // namespace freeusd::usdGeom
