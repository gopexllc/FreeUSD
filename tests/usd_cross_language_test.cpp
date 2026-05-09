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

  return 0;
}
