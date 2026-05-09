#include <cassert>
#include <vector>

#include "freeusd/pcp/layerStack.hpp"
#include "freeusd/sdf/layer.hpp"
#include "freeusd/sdf/path.hpp"
#include "freeusd/sdf/primReference.hpp"
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

  {
    freeusd::sdf::PrimReference ra;
    ra.asset_path = "./a.usda";
    strong->AddPrimReference(px, ra);
    freeusd::sdf::PrimReference rb;
    rb.asset_path = "./b.usda";
    weaker->AddPrimReference(px, rb);
    const auto refs = composed->ReadPrimReferences(px);
    assert(refs.size() == 2u);
    assert(refs[0].asset_path == "./a.usda");
    assert(refs[1].asset_path == "./b.usda");
    assert(px_prim.GetReferences().size() == 2u);

    strong->AddPrimInherit(px, Path::FromString("/BaseA"));
    weaker->AddPrimInherit(px, Path::FromString("/BaseB"));
    const auto inh = composed->ReadPrimInherits(px);
    assert(inh.size() == 2u);
    assert(inh[0] == Path::FromString("/BaseA"));
    assert(px_prim.GetInherits().size() == 2u);

    strong->AddPrimSpecializes(px, Path::FromString("/SpecA"));
    weaker->AddPrimSpecializes(px, Path::FromString("/SpecB"));
    const auto sp = composed->ReadPrimSpecializes(px);
    assert(sp.size() == 2u);
    assert(sp[0] == Path::FromString("/SpecA"));
    assert(px_prim.GetSpecializes().size() == 2u);
    assert(composed->HasPrimSpecializes(px));
    assert(px_prim.HasSpecializes());

    freeusd::sdf::PrimReference pl;
    pl.asset_path = "./p.usdc";
    strong->AddPrimPayload(px, pl);
    const auto pays = composed->ReadPrimPayloads(px);
    assert(pays.size() == 1u);
    assert(pays[0].asset_path == "./p.usdc");
    assert(px_prim.GetPayloads().size() == 1u);
  }

  const Path pk = Path::FromString("/Typed");
  weaker->SetPrimKind(pk, Token("Mesh"));
  strong->SetPrimKind(pk, Token("Xform"));
  const auto tk = composed->GetPrimAtPath(pk);
  assert(tk.HasPrimKind());
  assert(tk.GetPrimKind().GetText() == "Xform");

  weaker->SetPrimSpecifier(px, Layer::PrimSpecifierKind::Over);
  assert(composed->ResolvePrimSpecifierKind(px) == Layer::PrimSpecifierKind::Over);
  assert(!px_prim.IsAbstract());
  weaker->SetPrimSpecifier(pk, Layer::PrimSpecifierKind::Class);
  assert(composed->ResolvePrimSpecifierKind(pk) == Layer::PrimSpecifierKind::Class);
  assert(tk.IsAbstract());
  assert(tk.GetSpecifierKind() == Layer::PrimSpecifierKind::Class);

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

  strong->SetCustomLayerDataEntry("pipelineId", Value::MakeString("main"));
  weaker->SetCustomLayerDataEntry("pipelineId", Value::MakeString("alt"));
  assert(composed->GetComposedCustomLayerData("pipelineId", &tagv));
  assert(tagv.GetString(&ts));
  assert(ts == "main");
  weaker->SetCustomLayerDataEntry("layerOnly", Value::MakeInt32(1));
  assert(composed->CustomLayerDataKeyInAnyLayer("layerOnly"));
  const auto lkeys = composed->ListComposedCustomLayerDataKeys();
  assert(lkeys.size() >= 2u);

  weaker->SetPrimVariantSelectionEntry(px, "shape", "Lo");
  strong->SetPrimVariantSelectionEntry(px, "shape", "Hi");
  strong->SetPrimVariantSelectionEntry(px, "lod", "Full");
  std::string vs;
  assert(composed->GetComposedPrimVariantSelection(px, "shape", &vs));
  assert(vs == "Hi");
  assert(composed->GetComposedPrimVariantSelection(px, "lod", &vs));
  assert(vs == "Full");
  assert(composed->PrimVariantSelectionSetInAnyLayer(px, "shape"));
  const auto vsets = composed->ListComposedPrimVariantSelectionSets(px);
  assert(vsets.size() == 2u);
  assert(px_prim.HasVariantSelectionKey("shape"));
  assert(px_prim.GetVariantSelection("shape") == "Hi");
  assert(px_prim.GetVariantSelection("lod") == "Full");
  assert(px_prim.ListVariantSelectionSets().size() == 2u);

  weaker->SetPrimVariantSetVariants(px, "geo", {"Lo"});
  strong->SetPrimVariantSetVariants(px, "geo", {"Hi", "Med"});
  weaker->SetPrimVariantSetVariants(px, "solo_set", {"Only"});
  const auto set_names = composed->ListComposedPrimVariantSetNames(px);
  assert(set_names.size() == 2u);
  assert(composed->GetComposedPrimVariantNames(px, "geo").size() == 2u);
  assert(composed->GetComposedPrimVariantNames(px, "geo")[0] == "Hi");
  assert(composed->GetComposedPrimVariantNames(px, "geo")[1] == "Med");
  assert(composed->GetComposedPrimVariantNames(px, "solo_set").size() == 1u);
  assert(composed->GetComposedPrimVariantNames(px, "solo_set")[0] == "Only");
  assert(composed->PrimVariantSetDeclaredInAnyLayer(px, "geo"));
  assert(px_prim.HasVariantSet("geo"));
  assert(px_prim.ListVariantSetNames().size() == 2u);
  assert(px_prim.ListVariantNames("geo").size() == 2u);

  const Path src = Path::FromString("/RelocateSrc");
  const Path dst_strong = Path::FromString("/RelocateStrong");
  const Path dst_weak = Path::FromString("/RelocateWeak");
  strong->SetRelocate(src, dst_strong);
  weaker->SetRelocate(src, dst_weak);
  freeusd::sdf::Path got;
  assert(composed->GetComposedRelocateTarget(src, &got));
  assert(got == dst_strong);
  assert(composed->RelocateSourceInAnyLayer(src));
  const Path only_weak_src = Path::FromString("/OnlyWeakRelo");
  const Path only_weak_dst = Path::FromString("/OnlyWeakDst");
  weaker->SetRelocate(only_weak_src, only_weak_dst);
  assert(composed->GetComposedRelocateTarget(only_weak_src, &got));
  assert(got == only_weak_dst);
  const auto all_r = composed->ListComposedRelocates();
  assert(all_r.size() == 2u);

  strong->SetPrefixSubstitution("/Models", "/ModelsV2");
  weaker->SetPrefixSubstitution("/Models", "/ModelsWeak");
  weaker->SetPrefixSubstitution("/OnlyW", "/W");
  std::string psub;
  assert(composed->GetComposedPrefixSubstitution("/Models", &psub));
  assert(psub == "/ModelsV2");
  assert(composed->GetComposedPrefixSubstitution("/OnlyW", &psub));
  assert(psub == "/W");
  assert(composed->PrefixSubstitutionKeyInAnyLayer("/Models"));
  const auto all_p = composed->ListComposedPrefixSubstitutions();
  assert(all_p.size() == 2u);

  strong->SetStartTimeCode(1.0);
  strong->SetEndTimeCode(100.0);
  strong->SetTimeCodesPerSecond(24.0);
  strong->SetFramesPerSecond(24.0);
  strong->SetFramePrecision(3);
  strong->SetMetersPerUnit(0.01);
  strong->SetUpAxis("Y");
  strong->SetPrimOrder({Path::FromString("/Root/X"), Path::FromString("/Typed")});
  weaker->SetMetersPerUnit(1.0);
  weaker->SetUpAxis("Z");
  assert(composed->GetStartTimeCode().has_value() && *composed->GetStartTimeCode() == 1.0);
  assert(composed->GetEndTimeCode().has_value() && *composed->GetEndTimeCode() == 100.0);
  assert(composed->GetTimeCodesPerSecond().has_value() && *composed->GetTimeCodesPerSecond() == 24.0);
  assert(composed->GetFramesPerSecond().has_value() && *composed->GetFramesPerSecond() == 24.0);
  assert(composed->GetFramePrecision().has_value() && *composed->GetFramePrecision() == 3);
  assert(composed->GetMetersPerUnit().has_value() && *composed->GetMetersPerUnit() == 0.01);
  assert(composed->GetUpAxis().has_value() && *composed->GetUpAxis() == "Y");
  const auto po = composed->GetPrimOrder();
  assert(po.size() == 2u);
  assert(po[0] == Path::FromString("/Root/X"));
  assert(po[1] == Path::FromString("/Typed"));

  weaker->SetStartTimeCode(5.0);
  strong->ClearStartTimeCode();
  assert(composed->GetStartTimeCode().has_value() && *composed->GetStartTimeCode() == 5.0);
  strong->ClearMetersPerUnit();
  assert(composed->GetMetersPerUnit().has_value() && *composed->GetMetersPerUnit() == 1.0);
  strong->ClearUpAxis();
  assert(composed->GetUpAxis().has_value() && *composed->GetUpAxis() == "Z");
  strong->ClearPrimOrder();
  weaker->SetPrimOrder({Path::FromString("/OnlyWeakRelo")});
  const auto po_f = composed->GetPrimOrder();
  assert(po_f.size() == 1u);
  assert(po_f[0] == Path::FromString("/OnlyWeakRelo"));

  return 0;
}
