#include <cassert>

#include "freeusd/io/usda.hpp"
#include "freeusd/sdf/layer.hpp"
#include "freeusd/sdf/path.hpp"
#include "freeusd/tf/token.hpp"
#include "freeusd/vt/value.hpp"

int main() {
  using freeusd::io::usda::LoadFromString;
  using freeusd::io::usda::SaveToString;
  using freeusd::sdf::Layer;
  using freeusd::sdf::Path;
  using freeusd::tf::Token;
  using freeusd::vt::Value;

  const char* doc = R"USDA(
#usda 1.0
(
    doc = "test"
)

def Xform "World"
(
    prepend references = @./payload.usda@</Geometry/Mesh>
)
{
    def "Cube"
    {
        double size = 2.5
        double height.timeSamples = {
            1: 10,
            3: 30,
        }
        bool visible = true
        string doc = "hello"
        rel material:binding = </Materials/M>
        rel proxyPrim = </Materials/M>
        prepend rel proxyPrim = </Proxy/Draw>
    }
}
)USDA";

  auto layer = Layer::NewAnonymous("roundtrip");
  auto r = LoadFromString(doc, layer);
  assert(r.ok);

  const Path cube = Path::FromString("/World/Cube");
  Value v;
  assert(layer->GetField(cube, Token("size"), &v));
  double d = 0;
  assert(v.GetDouble(&d));
  assert(d == 2.5);

  assert(layer->GetFieldAtTime(cube, Token("height"), 2.0, &v));
  assert(v.GetDouble(&d));
  assert(d == 10.0);
  assert(layer->GetFieldAtTime(cube, Token("height"), 3.0, &v));
  assert(v.GetDouble(&d));
  assert(d == 30.0);

  const std::string out = SaveToString(*layer);
  auto layer2 = Layer::NewAnonymous("b");
  r = LoadFromString(out, layer2);
  assert(r.ok);
  assert(layer2->GetField(cube, Token("size"), &v));
  assert(v.GetDouble(&d));
  assert(d == 2.5);
  assert(layer2->GetFieldAtTime(cube, Token("height"), 2.0, &v));
  assert(v.GetDouble(&d));
  assert(d == 10.0);

  assert(layer2->HasPrimKind(Path::FromString("/World")));
  assert(layer2->GetPrimKind(Path::FromString("/World")).GetText() == "Xform");

  assert(layer2->HasRelationship(cube, Token("material:binding")));
  const auto mb = layer2->GetRelationshipTargets(cube, Token("material:binding"));
  assert(mb.size() == 1);
  assert(mb[0] == Path::FromString("/Materials/M"));
  const auto pp = layer2->GetRelationshipTargets(cube, Token("proxyPrim"));
  assert(pp.size() == 2);
  assert(pp[0] == Path::FromString("/Proxy/Draw"));
  assert(pp[1] == Path::FromString("/Materials/M"));

  const Path world = Path::FromString("/World");
  const auto wl = layer2->ListPrimReferences(world);
  assert(wl.size() == 1);
  assert(wl[0].asset_path == "./payload.usda");
  assert(wl[0].prim_path.has_value());
  assert(*wl[0].prim_path == Path::FromString("/Geometry/Mesh"));

  return 0;
}
