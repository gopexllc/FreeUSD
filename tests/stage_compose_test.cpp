#include <cassert>
#include <vector>

#include "freeusd/pcp/layerStack.hpp"
#include "freeusd/sdf/layer.hpp"
#include "freeusd/sdf/path.hpp"
#include "freeusd/tf/token.hpp"
#include "freeusd/usd/stage.hpp"
#include "freeusd/vt/value.hpp"

int main() {
  using freeusd::pcp::LayerStack;
  using freeusd::sdf::Layer;
  using freeusd::sdf::Path;
  using freeusd::tf::Token;
  using freeusd::usd::Stage;
  using freeusd::vt::Value;

  auto strong = Layer::NewAnonymous("strong");
  auto weaker = Layer::NewAnonymous("weaker");

  const Path px = Path::FromString("/Root/X");
  strong->SetField(px, Token("strength"), Value::MakeDouble(1.0));
  weaker->SetField(px, Token("strength"), Value::MakeDouble(99.0));

  LayerStack st;
  st.Append(strong);
  st.Append(weaker);

  auto composed = Stage::AttachLayerStack(st);
  assert(composed);
  assert(composed->GetComposeLayers().size() == 2);

  const auto px_prim = composed->GetPrimAtPath(px);
  assert(px_prim.IsValid());

  Value vattr = px_prim.GetAttribute(Token("strength"));
  double d = 0;
  assert(vattr.GetDouble(&d));
  assert(d == 1.0);

  const Path only_weak = Path::FromString("/Root/WeakOnly");
  weaker->SetField(only_weak, Token("solo"), Value::MakeInt32(42));
  const auto ow = composed->GetPrimAtPath(only_weak);
  assert(ow.IsValid());
  Value vw = ow.GetAttribute(Token("solo"));
  assert(vw.GetDouble(&d));
  assert(d == 42.0);

  strong->SetRelationshipTargets(px, Token("tie"),
                                 std::vector<Path>{Path::FromString("/OnStrong")});
  weaker->SetRelationshipTargets(px, Token("tie"),
                                 std::vector<Path>{Path::FromString("/OnWeak")});
  const auto ties = px_prim.GetRelationshipTargets(Token("tie"));
  assert(ties.size() == 2);
  assert(ties[0] == Path::FromString("/OnStrong"));
  assert(ties[1] == Path::FromString("/OnWeak"));

  const Path pk = Path::FromString("/Typed");
  weaker->SetPrimKind(pk, Token("Mesh"));
  strong->SetPrimKind(pk, Token("Xform"));
  const auto tk = composed->GetPrimAtPath(pk);
  assert(tk.HasPrimKind());
  assert(tk.GetPrimKind().GetText() == "Xform");

  strong->SetPrimActive(px, false);
  assert(px_prim.HasPrimActiveOpinion());
  assert(px_prim.IsActive() == false);

  weaker->SetPrimCustomDataEntry(px, "tag", Value::MakeString("weak"));
  strong->SetPrimCustomDataEntry(px, "tag", Value::MakeString("strong"));
  Value tagv;
  assert(composed->GetComposedPrimCustomData(px, "tag", &tagv));
  std::string ts;
  assert(tagv.GetString(&ts));
  assert(ts == "strong");

  weaker->SetPrimCustomDataEntry(px, "onlyWeak", Value::MakeInt32(7));
  const auto ckeys = composed->ListComposedPrimCustomDataKeys(px);
  bool has_only = false;
  for (const auto& k : ckeys) {
    if (k == "onlyWeak") {
      has_only = true;
    }
  }
  assert(has_only);
  assert(px_prim.HasCustomDataKey("onlyWeak"));
  assert(px_prim.ListCustomDataKeys().size() >= 2u);

  return 0;
}
