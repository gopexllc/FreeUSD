#include <cmath>
#include <fstream>
#include <sstream>
#include <string>

#include "freeusd/io/usda.hpp"
#include "freeusd/sdf/layer.hpp"
#include "freeusd/sdf/path.hpp"
#include "freeusd/tf/token.hpp"
#include "freeusd/usd/stage.hpp"
#include "freeusd/vt/value.hpp"

#ifndef FREEUSD_TEST_FIXTURES_DIR
#error "FREEUSD_TEST_FIXTURES_DIR must be set by CMake for usd_cross_language_test"
#endif

#define REQ(cond, code) \
  do { \
    if (!(cond)) return (code); \
  } while (0)

int main() {
  using freeusd::io::usda::LoadFromString;
  using freeusd::sdf::Layer;
  using freeusd::sdf::Path;
  using freeusd::tf::Token;
  using freeusd::usd::Stage;
  using freeusd::vt::Value;

  const std::string fixture_path = std::string(FREEUSD_TEST_FIXTURES_DIR) + "/usd_cross_language.usda";
  std::ifstream in(fixture_path);
  REQ(in, 2);
  std::ostringstream ss;
  ss << in.rdbuf();
  const std::string text = ss.str();
  REQ(!text.empty(), 3);

  auto layer = Layer::NewAnonymous("cross_lang_fixture");
  const auto pr = LoadFromString(text, layer);
  REQ(pr.ok, 4);

  REQ(layer->GetDocumentation() == "cross_lang_fixture", 5);
  REQ(layer->GetDefaultPrim().has_value() && *layer->GetDefaultPrim() == "Scene", 6);
  REQ(layer->GetMetersPerUnit().has_value() && *layer->GetMetersPerUnit() == 0.01, 7);
  REQ(layer->GetUpAxis().has_value() && *layer->GetUpAxis() == "Y", 8);
  REQ(layer->GetStartTimeCode().has_value() && *layer->GetStartTimeCode() == 0.0, 9);
  REQ(layer->GetEndTimeCode().has_value() && *layer->GetEndTimeCode() == 100.0, 10);

  auto stage = Stage::AttachRootLayer(layer);
  REQ(static_cast<bool>(stage), 11);
  REQ(stage->HasDefaultPrim(), 12);
  REQ(stage->GetDefaultPrimName() == "Scene", 13);
  REQ(stage->GetMetersPerUnit().has_value() && *stage->GetMetersPerUnit() == 0.01, 14);
  REQ(stage->GetUpAxis().has_value() && *stage->GetUpAxis() == "Y", 15);
  REQ(stage->GetStartTimeCode().has_value() && *stage->GetStartTimeCode() == 0.0, 16);
  REQ(stage->GetEndTimeCode().has_value() && *stage->GetEndTimeCode() == 100.0, 17);

  const Path child = Path::FromString("/Scene/Child");
  const auto prim = stage->GetPrimAtPath(child);
  REQ(prim.IsValid(), 18);
  REQ(prim.GetName() == "Child", 19);

  Value tag;
  REQ(stage->GetComposedPrimCustomData(child, "tag", &tag), 20);
  std::int32_t tag_i = 0;
  REQ(tag.GetInt32(&tag_i) && tag_i == 99, 21);

  double d = 0;
  const Value m1 = prim.GetAttribute(Token("mass"), 1.0);
  REQ(m1.GetDouble(&d) && d == 2.5, 22);
  const Value m2 = prim.GetAttribute(Token("mass"), 2.0);
  REQ(m2.GetDouble(&d) && d == 4.0, 23);

  REQ(stage->HasFieldOpinion(child, Token("mass")), 24);
  REQ(!stage->HasFieldOpinion(child, Token("missingAttr")), 25);
  REQ(prim.HasAttribute(Token("mass")), 26);
  REQ(!prim.HasAttribute(Token("missingAttr")), 27);

  float density = 0.0F;
  const Value density_v = prim.GetAttribute(Token("density"), 1.0);
  REQ(density_v.GetFloat(&density) && std::abs(density - 1.25F) < 1e-6F, 28);

  bool enabled = false;
  Value enabled_v;
  REQ(stage->ReadFieldAtEvaluatedTime(child, Token("enabled"), 1.0, &enabled_v), 29);
  REQ(enabled_v.GetBool(&enabled) && enabled, 30);

  std::int32_t count = 0;
  const Value count_v = prim.GetAttribute(Token("count"), 1.0);
  REQ(count_v.GetInt32(&count) && count == -7, 31);

  std::string label;
  const Value label_v = prim.GetAttribute(Token("label"), 1.0);
  REQ(label_v.GetString(&label) && label == "hello", 32);

  freeusd::tf::Token kind;
  const Value kind_v = prim.GetAttribute(Token("kind"), 1.0);
  REQ(kind_v.GetToken(&kind) && kind.GetText() == "component", 33);

  std::vector<freeusd::tf::Token> tags;
  const Value tags_v = prim.GetAttribute(Token("tags"), 1.0);
  REQ(tags_v.GetTokenArray(&tags) && tags.size() == 2 && tags[0].GetText() == "a" && tags[1].GetText() == "b", 34);

  freeusd::gf::Vec3d extent;
  Value extent_v;
  REQ(stage->ReadFieldAtEvaluatedTime(child, Token("extent"), 1.0, &extent_v), 35);
  REQ(extent_v.GetVec3d(&extent) && extent.x() == 1.0 && extent.y() == 2.0 && extent.z() == 3.0, 36);

  freeusd::gf::Vec3f display_color;
  const Value display_color_v = prim.GetAttribute(Token("displayColor"), 1.0);
  REQ(display_color_v.GetVec3f(&display_color) && std::abs(display_color.x() - 0.25F) < 1e-6F &&
          std::abs(display_color.y() - 0.5F) < 1e-6F && std::abs(display_color.z() - 0.75F) < 1e-6F,
      37);

  freeusd::gf::Matrix4d xf;
  const Value xf_v = prim.GetAttribute(Token("xf"), 1.0);
  REQ(xf_v.GetMatrix4d(&xf) && xf == freeusd::gf::Matrix4d::Identity(), 38);

  freeusd::gf::Quatd qd;
  const Value qd_v = prim.GetAttribute(Token("qd"), 1.0);
  REQ(qd_v.GetQuatd(&qd) && qd == freeusd::gf::Quatd::Identity(), 39);

  freeusd::gf::Quatf qf;
  Value qf_v;
  REQ(stage->ReadFieldAtEvaluatedTime(child, Token("qf"), 1.0, &qf_v), 40);
  REQ(qf_v.GetQuatf(&qf) && std::abs(qf.real - 0.70710677F) < 1e-5F && std::abs(qf.k - 0.70710677F) < 1e-5F, 41);

  const auto partial = stage->GetPrimAtPath(Path::FromString("/Scene/Partial"));
  REQ(partial.IsValid(), 42);
  REQ(partial.GetAttribute(Token("missingAttr"), 1.0).IsEmpty(), 43);

  const auto missing_prim = stage->GetPrimAtPath(Path::FromString("/Scene/Missing"));
  REQ(!missing_prim.IsValid(), 44);

  Value missing_value;
  REQ(!stage->ReadFieldAtEvaluatedTime(child, Token("missingAttr"), 1.0, &missing_value), 45);
  REQ(!stage->ReadFieldAtEvaluatedTime(Path::FromString("/Scene/Missing"), Token("mass"), 1.0, &missing_value), 46);

  return 0;
}
