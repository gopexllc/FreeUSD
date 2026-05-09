#include <cassert>

#include "freeusd/sdf/layer.hpp"
#include "freeusd/sdf/path.hpp"
#include "freeusd/tf/token.hpp"
#include "freeusd/usd/stage.hpp"
#include "freeusd/vt/value.hpp"

int main() {
  using freeusd::sdf::Layer;
  using freeusd::sdf::Path;
  using freeusd::tf::Token;
  using freeusd::usd::Stage;
  using freeusd::vt::Value;

  auto layer = Layer::NewAnonymous("anon");
  const Path world = Path::FromString("/World");
  const Path cube = Path::FromString("/World/Cube");
  layer->SetField(cube, Token("size"), Value::MakeDouble(2.5));

  auto stage = Stage::AttachRootLayer(layer);
  assert(stage);
  const auto kids = stage->GetChildren(Path::AbsoluteRootPath());
  assert(!kids.empty());

  const auto prim = stage->GetPrimAtPath(cube);
  assert(prim.IsValid());
  assert(prim.HasAttribute(Token("size")));
  double d = 0;
  assert(prim.GetAttribute(Token("size")).GetDouble(&d));
  assert(d == 2.5);

  return 0;
}
