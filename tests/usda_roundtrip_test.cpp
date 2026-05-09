#include <cassert>

#include "freeusd/io/usda.hpp"
#include "freeusd/sdf/layer.hpp"
#include "freeusd/sdf/path.hpp"
#include "freeusd/tf/token.hpp"
#include "freeusd/usd/stage.hpp"
#include "freeusd/vt/value.hpp"

int main() {
  using freeusd::io::usda::LoadFromString;
  using freeusd::io::usda::SaveToString;
  using freeusd::sdf::Layer;
  using freeusd::sdf::Path;
  using freeusd::tf::Token;
  using freeusd::usd::Stage;
  using freeusd::vt::Value;

  const char* doc = R"USDA(
#usda 1.0
(
    doc = "test"
    relocates = {
        </World/Pipe>: </World/Sink>,
    }
)

def Xform "World"
(
    prepend references = @./payload.usda@</Geometry/Mesh>
)
{
    def "Pipe"
    {
        double sourceValue = 2.718281828
    }
    def "Sink"
    {
        double ported.connect = </World/Pipe.sourceValue>
    }

    def "VPrim"
    (
        variantSelection = {
            string shapeVariant = "Sphere",
            token lodVariant = Hi,
        }
    )
    {
    }

    def "VariantHost"
    (
        variantSets = {
            shapeVariant = {
                "Sphere" = {},
                Cube = {},
            }
        }
        variantSelection = {
            string shapeVariant = "Sphere",
        }
    )
    {
    }

    def "Cube"
    (
        customData = {
            string pipeline = "cache",
            int build = 9,
        }
    )
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
        rel chains = </C1>
        append rel chains = </C2>
        prepend rel chains = </C0>
        rel dropme = [</X>, </Y>, </Z>]
        delete rel dropme = </Y>
        rel dup = [</A>, </A>, </B>]
        delete rel dup = </A>
    }
}
)USDA";

  auto layer = Layer::NewAnonymous("roundtrip");
  auto r = LoadFromString(doc, layer);
  assert(r.ok);

  const Path pipe = Path::FromString("/World/Pipe");
  const Path sink = Path::FromString("/World/Sink");
  freeusd::sdf::Path rel_to;
  assert(layer->HasRelocate(pipe));
  assert(layer->GetRelocateTarget(pipe, &rel_to));
  assert(rel_to == sink);

  const Path vprim = Path::FromString("/World/VPrim");
  assert(layer->HasPrimVariantSelectionKey(vprim, "shapeVariant"));
  std::string vsel;
  assert(layer->GetPrimVariantSelectionEntry(vprim, "shapeVariant", &vsel));
  assert(vsel == "Sphere");
  assert(layer->GetPrimVariantSelectionEntry(vprim, "lodVariant", &vsel));
  assert(vsel == "Hi");

  const Path vhost = Path::FromString("/World/VariantHost");
  assert(layer->HasPrimVariantSet(vhost, "shapeVariant"));
  const auto vnames = layer->ListPrimVariantNames(vhost, "shapeVariant");
  assert(vnames.size() == 2u);
  assert(vnames[0] == "Sphere");
  assert(vnames[1] == "Cube");
  assert(layer->GetPrimVariantSelectionEntry(vhost, "shapeVariant", &vsel));
  assert(vsel == "Sphere");

  const Path cube = Path::FromString("/World/Cube");
  assert(layer->HasPrimCustomDataKey(cube, "pipeline"));
  Value ctmp;
  std::string pv;
  assert(layer->GetPrimCustomDataEntry(cube, "pipeline", &ctmp));
  assert(ctmp.GetString(&pv));
  assert(pv == "cache");
  assert(layer->GetPrimCustomDataEntry(cube, "build", &ctmp));
  double bd = 0;
  assert(ctmp.GetDouble(&bd));
  assert(bd == 9.0);

  assert(layer->HasAttributeConnection(sink, Token("ported")));
  freeusd::sdf::Path conn_target;
  assert(layer->GetAttributeConnectionTarget(sink, Token("ported"), &conn_target));
  assert(conn_target == Path::FromString("/World/Pipe.sourceValue"));
  auto st0 = Stage::AttachRootLayer(layer);
  assert(st0);
  const auto sink_prim = st0->GetPrimAtPath(sink);
  assert(sink_prim.IsValid());
  Value fed = sink_prim.GetAttribute(Token("ported"));
  double d = 0;
  assert(fed.GetDouble(&d));
  assert(d > 2.718 && d < 2.719);

  Value v;
  assert(layer->GetField(cube, Token("size"), &v));
  d = 0;
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

  const auto ch = layer2->GetRelationshipTargets(cube, Token("chains"));
  assert(ch.size() == 3);
  assert(ch[0] == Path::FromString("/C0"));
  assert(ch[1] == Path::FromString("/C1"));
  assert(ch[2] == Path::FromString("/C2"));

  const auto dm = layer2->GetRelationshipTargets(cube, Token("dropme"));
  assert(dm.size() == 2);
  assert(dm[0] == Path::FromString("/X"));
  assert(dm[1] == Path::FromString("/Z"));

  const auto dup = layer2->GetRelationshipTargets(cube, Token("dup"));
  assert(dup.size() == 1);
  assert(dup[0] == Path::FromString("/B"));

  assert(layer2->HasPrimCustomDataKey(cube, "pipeline"));
  assert(layer2->GetPrimCustomDataEntry(cube, "build", &ctmp));
  assert(ctmp.GetDouble(&bd));
  assert(bd == 9.0);

  assert(layer2->HasPrimVariantSelectionKey(vprim, "shapeVariant"));
  assert(layer2->GetPrimVariantSelectionEntry(vprim, "lodVariant", &vsel));
  assert(vsel == "Hi");

  assert(layer2->HasPrimVariantSet(vhost, "shapeVariant"));
  const auto vnames2 = layer2->ListPrimVariantNames(vhost, "shapeVariant");
  assert(vnames2.size() == 2u);
  assert(vnames2[0] == "Sphere");
  assert(vnames2[1] == "Cube");

  assert(layer2->HasRelocate(pipe));
  assert(layer2->GetRelocateTarget(pipe, &rel_to));
  assert(rel_to == sink);

  assert(layer2->HasAttributeConnection(Path::FromString("/World/Sink"), Token("ported")));
  auto st1 = Stage::AttachRootLayer(layer2);
  assert(st1->GetPrimAtPath(sink).GetAttribute(Token("ported")).GetDouble(&d));
  assert(d > 2.718 && d < 2.719);

  const Path world = Path::FromString("/World");
  const auto wl = layer2->ListPrimReferences(world);
  assert(wl.size() == 1);
  assert(wl[0].asset_path == "./payload.usda");
  assert(wl[0].prim_path.has_value());
  assert(*wl[0].prim_path == Path::FromString("/Geometry/Mesh"));

  return 0;
}
