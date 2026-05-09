#include "freeusd/usd/stage.hpp"

#include <algorithm>
#include <filesystem>
#include <functional>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "freeusd/ar/defaultResolver.hpp"
#include "freeusd/io/usda.hpp"
#include "freeusd/pcp/compose.hpp"
#include "freeusd/pcp/resolve.hpp"
#include "freeusd/sdf/layer.hpp"
#include "freeusd/sdf/path.hpp"
#include "freeusd/tf/token.hpp"
#include "freeusd/vt/value.hpp"

namespace freeusd::usd {

namespace {

constexpr int kAttrConnectionHopLimit = 64;

bool ResolveAttributeConnectionStrongestFirst(const std::vector<std::shared_ptr<freeusd::sdf::Layer>>& strongest_first,
                                              const freeusd::sdf::Path& prim_path, const freeusd::tf::Token& name,
                                              freeusd::sdf::Path* target_prop_path) {
  if (!target_prop_path || name.IsEmpty()) {
    return false;
  }
  for (const std::shared_ptr<freeusd::sdf::Layer>& L : strongest_first) {
    if (!L) {
      continue;
    }
    if (L->HasAttributeConnection(prim_path, name)) {
      return L->GetAttributeConnectionTarget(prim_path, name, target_prop_path);
    }
  }
  return false;
}

}  // namespace

Stage::Stage(std::vector<std::shared_ptr<freeusd::sdf::Layer>> compose)
    : compose_(std::move(compose)),
      resolver_(std::make_unique<freeusd::ar::DefaultResolver>()) {}

std::shared_ptr<Stage> Stage::AttachRootLayer(std::shared_ptr<freeusd::sdf::Layer> root) {
  if (!root) {
    return {};
  }
  std::vector<std::shared_ptr<freeusd::sdf::Layer>> one;
  one.push_back(std::move(root));
  return std::shared_ptr<Stage>(new Stage(std::move(one)));
}

std::shared_ptr<Stage> Stage::AttachLayerStack(freeusd::pcp::LayerStack stack) {
  std::vector<std::shared_ptr<freeusd::sdf::Layer>> lys;
  lys.reserve(stack.GetLayers().size());
  for (const std::shared_ptr<freeusd::sdf::Layer>& L : stack.GetLayers()) {
    lys.push_back(L);
  }
  if (lys.empty()) {
    return {};
  }
  return std::shared_ptr<Stage>(new Stage(std::move(lys)));
}

std::shared_ptr<Stage> Stage::OpenFromRootFile(const std::string& layer_path, RootLayerSublayersPolicy sublayers,
                                               std::string* err_detail) {
  if (err_detail) {
    err_detail->clear();
  }
  namespace fs = std::filesystem;
  std::error_code ec;
  const fs::path raw{layer_path};
  if (!fs::exists(raw, ec) || ec) {
    if (err_detail) {
      *err_detail = "layer file not found: " + layer_path;
    }
    return {};
  }
  const fs::path lp = fs::weakly_canonical(raw, ec);
  if (ec) {
    if (err_detail) {
      *err_detail = ec.message();
    }
    return {};
  }
  std::string anchor;
  if (lp.has_parent_path()) {
    anchor = lp.parent_path().string();
  } else {
    anchor = fs::current_path().string();
  }

  auto root = freeusd::sdf::Layer::NewAnonymous(lp.filename().string());
  root->SetIdentifier(lp.string());
  const auto pr = freeusd::io::usda::LoadFromFile(lp.string(), root);
  if (!pr.ok) {
    if (err_detail) {
      *err_detail = "USDA parse error: " + pr.message + " (line " + std::to_string(pr.line) + ")";
    }
    return {};
  }

  if (sublayers == RootLayerSublayersPolicy::None) {
    return AttachRootLayer(std::move(root));
  }

  freeusd::ar::DefaultResolver resolver(anchor);
  auto resolve = [resolver](const std::string& authored) mutable -> std::shared_ptr<freeusd::sdf::Layer> {
    const std::string abs = resolver.Resolve(std::string_view{authored});
    if (abs.empty()) {
      return {};
    }
    std::error_code e2;
    const fs::path ap{abs};
    if (!fs::exists(ap, e2) || e2) {
      return {};
    }
    const fs::path canon = fs::weakly_canonical(ap, e2);
    if (e2) {
      return {};
    }
    auto L = freeusd::sdf::Layer::NewAnonymous(canon.filename().string());
    L->SetIdentifier(canon.string());
    const auto pr2 = freeusd::io::usda::LoadFromFile(canon.string(), L);
    if (!pr2.ok) {
      return {};
    }
    return L;
  };

  if (sublayers == RootLayerSublayersPolicy::Shallow) {
    return AttachLayerStack(freeusd::pcp::ComposeSublayers(root, resolve));
  }
  if (sublayers == RootLayerSublayersPolicy::DepthFirst) {
    return AttachLayerStack(freeusd::pcp::ComposeSublayersDepthFirst(root, resolve));
  }
  return AttachRootLayer(std::move(root));
}

bool Stage::ReadFieldAtEvaluatedTime(const freeusd::sdf::Path& prim_path, const freeusd::tf::Token& name, double time,
                                     freeusd::vt::Value* out) const {
  if (!out || name.IsEmpty()) {
    return false;
  }
  freeusd::sdf::Path cur_prim = prim_path;
  freeusd::tf::Token cur_name = name;
  std::unordered_set<std::string> seen;
  for (int hops = 0; hops <= kAttrConnectionHopLimit; ++hops) {
    const std::string step = cur_prim.GetString() + '\x1f' + cur_name.GetText();
    if (!seen.insert(step).second) {
      return false;
    }
    freeusd::sdf::Path conn_to;
    if (!ResolveAttributeConnectionStrongestFirst(compose_, cur_prim, cur_name, &conn_to)) {
      return freeusd::pcp::ResolveFieldAtTimeStrongestWins(compose_, cur_prim, cur_name, time, out);
    }
    if (!conn_to.IsPropertyPath()) {
      return false;
    }
    cur_prim = conn_to.GetPrimPath();
    cur_name = freeusd::tf::Token{conn_to.GetName()};
    if (cur_name.IsEmpty()) {
      return false;
    }
  }
  return false;
}

bool Stage::HasFieldOpinion(const freeusd::sdf::Path& prim_path, const freeusd::tf::Token& name) const {
  if (freeusd::pcp::HasFieldOpinionOnAnyLayer(compose_, prim_path, name)) {
    return true;
  }
  for (const std::shared_ptr<freeusd::sdf::Layer>& L : compose_) {
    if (L && L->HasAttributeConnection(prim_path, name)) {
      return true;
    }
  }
  return false;
}

bool Stage::HasAttributeConnection(const freeusd::sdf::Path& prim_path, const freeusd::tf::Token& attr_name) const {
  if (attr_name.IsEmpty()) {
    return false;
  }
  for (const std::shared_ptr<freeusd::sdf::Layer>& L : compose_) {
    if (L && L->HasAttributeConnection(prim_path, attr_name)) {
      return true;
    }
  }
  return false;
}

bool Stage::GetComposedAttributeConnectionTarget(const freeusd::sdf::Path& prim_path,
                                                 const freeusd::tf::Token& attr_name,
                                                 freeusd::sdf::Path* out_target) const {
  if (!out_target || attr_name.IsEmpty()) {
    return false;
  }
  return ResolveAttributeConnectionStrongestFirst(compose_, prim_path, attr_name, out_target);
}

bool Stage::PrimPathInUse(const freeusd::sdf::Path& path) const {
  if (!path.IsPrimPath()) {
    return false;
  }
  for (const std::shared_ptr<freeusd::sdf::Layer>& L : compose_) {
    if (!L) {
      continue;
    }
    for (const auto& q : L->ListPrimPaths()) {
      if (q == path) {
        return true;
      }
    }
  }
  return false;
}

bool Stage::ReadRelationship(const freeusd::sdf::Path& prim_path, const freeusd::tf::Token& rel_name,
                             std::vector<freeusd::sdf::Path>* out_targets) const {
  if (!out_targets || rel_name.IsEmpty()) {
    return false;
  }
  out_targets->clear();
  bool any = false;
  for (const std::shared_ptr<freeusd::sdf::Layer>& L : compose_) {
    if (!L || !L->HasRelationship(prim_path, rel_name)) {
      continue;
    }
    const std::vector<freeusd::sdf::Path> part = L->GetRelationshipTargets(prim_path, rel_name);
    out_targets->insert(out_targets->end(), part.begin(), part.end());
    any = true;
  }
  return any;
}

bool Stage::HasRelationship(const freeusd::sdf::Path& prim_path, const freeusd::tf::Token& rel_name) const {
  if (rel_name.IsEmpty()) {
    return false;
  }
  for (const std::shared_ptr<freeusd::sdf::Layer>& L : compose_) {
    if (L && L->HasRelationship(prim_path, rel_name)) {
      return true;
    }
  }
  return false;
}

std::vector<freeusd::sdf::PrimReference> Stage::ReadPrimReferences(const freeusd::sdf::Path& prim_path) const {
  std::vector<freeusd::sdf::PrimReference> out;
  for (const std::shared_ptr<freeusd::sdf::Layer>& L : compose_) {
    if (!L) {
      continue;
    }
    const std::vector<freeusd::sdf::PrimReference> part = L->ListPrimReferences(prim_path);
    out.insert(out.end(), part.begin(), part.end());
  }
  return out;
}

bool Stage::HasPrimReferences(const freeusd::sdf::Path& prim_path) const {
  return !ReadPrimReferences(prim_path).empty();
}

std::vector<freeusd::sdf::Path> Stage::ReadPrimInherits(const freeusd::sdf::Path& prim_path) const {
  std::vector<freeusd::sdf::Path> out;
  for (const std::shared_ptr<freeusd::sdf::Layer>& L : compose_) {
    if (!L) {
      continue;
    }
    const std::vector<freeusd::sdf::Path> part = L->ListPrimInherits(prim_path);
    out.insert(out.end(), part.begin(), part.end());
  }
  return out;
}

bool Stage::HasPrimInherits(const freeusd::sdf::Path& prim_path) const {
  return !ReadPrimInherits(prim_path).empty();
}

std::vector<freeusd::sdf::Path> Stage::ReadPrimSpecializes(const freeusd::sdf::Path& prim_path) const {
  std::vector<freeusd::sdf::Path> out;
  for (const std::shared_ptr<freeusd::sdf::Layer>& L : compose_) {
    if (!L) {
      continue;
    }
    const std::vector<freeusd::sdf::Path> part = L->ListPrimSpecializes(prim_path);
    out.insert(out.end(), part.begin(), part.end());
  }
  return out;
}

bool Stage::HasPrimSpecializes(const freeusd::sdf::Path& prim_path) const {
  return !ReadPrimSpecializes(prim_path).empty();
}

std::vector<freeusd::sdf::PrimReference> Stage::ReadPrimPayloads(const freeusd::sdf::Path& prim_path) const {
  std::vector<freeusd::sdf::PrimReference> out;
  for (const std::shared_ptr<freeusd::sdf::Layer>& L : compose_) {
    if (!L) {
      continue;
    }
    const std::vector<freeusd::sdf::PrimReference> part = L->ListPrimPayloads(prim_path);
    out.insert(out.end(), part.begin(), part.end());
  }
  return out;
}

bool Stage::HasPrimPayloads(const freeusd::sdf::Path& prim_path) const {
  return !ReadPrimPayloads(prim_path).empty();
}

freeusd::tf::Token Stage::ResolvePrimKind(const freeusd::sdf::Path& prim_path) const {
  for (const std::shared_ptr<freeusd::sdf::Layer>& L : compose_) {
    if (L && L->HasPrimKind(prim_path)) {
      return L->GetPrimKind(prim_path);
    }
  }
  return {};
}

bool Stage::ResolveHasPrimKind(const freeusd::sdf::Path& prim_path) const {
  for (const std::shared_ptr<freeusd::sdf::Layer>& L : compose_) {
    if (L && L->HasPrimKind(prim_path)) {
      return true;
    }
  }
  return false;
}

freeusd::sdf::Layer::PrimSpecifierKind Stage::ResolvePrimSpecifierKind(const freeusd::sdf::Path& prim_path) const {
  for (const std::shared_ptr<freeusd::sdf::Layer>& L : compose_) {
    if (L && L->HasPrimSpecifierOpinion(prim_path)) {
      return L->GetPrimSpecifier(prim_path);
    }
  }
  return freeusd::sdf::Layer::PrimSpecifierKind::Default;
}

bool Stage::ResolvePrimActive(const freeusd::sdf::Path& prim_path) const {
  for (const std::shared_ptr<freeusd::sdf::Layer>& L : compose_) {
    if (L && L->HasPrimActiveOpinion(prim_path)) {
      return L->IsPrimActive(prim_path);
    }
  }
  return true;
}

bool Stage::ResolveHasPrimActiveOpinion(const freeusd::sdf::Path& prim_path) const {
  for (const std::shared_ptr<freeusd::sdf::Layer>& L : compose_) {
    if (L && L->HasPrimActiveOpinion(prim_path)) {
      return true;
    }
  }
  return false;
}

bool Stage::GetComposedPrimCustomData(const freeusd::sdf::Path& prim_path, const std::string& key,
                                      freeusd::vt::Value* out) const {
  if (!out || key.empty()) {
    return false;
  }
  for (const std::shared_ptr<freeusd::sdf::Layer>& L : compose_) {
    if (L && L->HasPrimCustomDataKey(prim_path, key)) {
      return L->GetPrimCustomDataEntry(prim_path, key, out);
    }
  }
  return false;
}

bool Stage::PrimCustomDataKeyInAnyLayer(const freeusd::sdf::Path& prim_path, const std::string& key) const {
  if (key.empty()) {
    return false;
  }
  for (const std::shared_ptr<freeusd::sdf::Layer>& L : compose_) {
    if (L && L->HasPrimCustomDataKey(prim_path, key)) {
      return true;
    }
  }
  return false;
}

std::vector<std::string> Stage::ListComposedPrimCustomDataKeys(const freeusd::sdf::Path& prim_path) const {
  std::set<std::string> keys;
  for (const std::shared_ptr<freeusd::sdf::Layer>& L : compose_) {
    if (!L) {
      continue;
    }
    for (const std::string& k : L->ListPrimCustomDataKeys(prim_path)) {
      keys.insert(k);
    }
  }
  return std::vector<std::string>(keys.begin(), keys.end());
}

bool Stage::GetComposedCustomLayerData(const std::string& key, freeusd::vt::Value* out) const {
  if (!out || key.empty()) {
    return false;
  }
  for (const std::shared_ptr<freeusd::sdf::Layer>& L : compose_) {
    if (L && L->HasCustomLayerDataKey(key)) {
      return L->GetCustomLayerDataEntry(key, out);
    }
  }
  return false;
}

bool Stage::CustomLayerDataKeyInAnyLayer(const std::string& key) const {
  if (key.empty()) {
    return false;
  }
  for (const std::shared_ptr<freeusd::sdf::Layer>& L : compose_) {
    if (L && L->HasCustomLayerDataKey(key)) {
      return true;
    }
  }
  return false;
}

std::vector<std::string> Stage::ListComposedCustomLayerDataKeys() const {
  std::set<std::string> keys;
  for (const std::shared_ptr<freeusd::sdf::Layer>& L : compose_) {
    if (!L) {
      continue;
    }
    for (const std::string& k : L->ListCustomLayerDataKeys()) {
      keys.insert(k);
    }
  }
  return std::vector<std::string>(keys.begin(), keys.end());
}

bool Stage::GetComposedPrimVariantSelection(const freeusd::sdf::Path& prim_path, const std::string& variantSet,
                                            std::string* outName) const {
  if (!outName || variantSet.empty()) {
    return false;
  }
  for (const std::shared_ptr<freeusd::sdf::Layer>& L : compose_) {
    if (L && L->HasPrimVariantSelectionKey(prim_path, variantSet)) {
      return L->GetPrimVariantSelectionEntry(prim_path, variantSet, outName);
    }
  }
  return false;
}

bool Stage::PrimVariantSelectionSetInAnyLayer(const freeusd::sdf::Path& prim_path,
                                              const std::string& variantSet) const {
  if (variantSet.empty()) {
    return false;
  }
  for (const std::shared_ptr<freeusd::sdf::Layer>& L : compose_) {
    if (L && L->HasPrimVariantSelectionKey(prim_path, variantSet)) {
      return true;
    }
  }
  return false;
}

std::vector<std::string> Stage::ListComposedPrimVariantSelectionSets(const freeusd::sdf::Path& prim_path) const {
  std::set<std::string> keys;
  for (const std::shared_ptr<freeusd::sdf::Layer>& L : compose_) {
    if (!L) {
      continue;
    }
    for (const std::string& k : L->ListPrimVariantSelectionSets(prim_path)) {
      keys.insert(k);
    }
  }
  return std::vector<std::string>(keys.begin(), keys.end());
}

std::vector<std::string> Stage::ListComposedPrimVariantSetNames(const freeusd::sdf::Path& prim_path) const {
  std::set<std::string> names;
  for (const std::shared_ptr<freeusd::sdf::Layer>& L : compose_) {
    if (!L) {
      continue;
    }
    for (const std::string& n : L->ListPrimVariantSetNames(prim_path)) {
      names.insert(n);
    }
  }
  return std::vector<std::string>(names.begin(), names.end());
}

bool Stage::PrimVariantSetDeclaredInAnyLayer(const freeusd::sdf::Path& prim_path,
                                             const std::string& variantSetName) const {
  if (variantSetName.empty()) {
    return false;
  }
  for (const std::shared_ptr<freeusd::sdf::Layer>& L : compose_) {
    if (L && L->HasPrimVariantSet(prim_path, variantSetName)) {
      return true;
    }
  }
  return false;
}

std::vector<std::string> Stage::GetComposedPrimVariantNames(const freeusd::sdf::Path& prim_path,
                                                            const std::string& variantSetName) const {
  if (variantSetName.empty()) {
    return {};
  }
  for (const std::shared_ptr<freeusd::sdf::Layer>& L : compose_) {
    if (L && L->HasPrimVariantSet(prim_path, variantSetName)) {
      return L->ListPrimVariantNames(prim_path, variantSetName);
    }
  }
  return {};
}

std::vector<std::string> Stage::ListComposedFieldNames(const freeusd::sdf::Path& prim_path) const {
  std::set<std::string> keys;
  for (const std::shared_ptr<freeusd::sdf::Layer>& L : compose_) {
    if (!L) {
      continue;
    }
    for (const std::string& k : L->ListFieldNames(prim_path)) {
      keys.insert(k);
    }
  }
  return std::vector<std::string>(keys.begin(), keys.end());
}

std::vector<double> Stage::ListComposedFieldSampleTimes(const freeusd::sdf::Path& prim_path,
                                                         const freeusd::tf::Token& name) const {
  std::set<double> times;
  if (!name.IsEmpty()) {
    for (const std::shared_ptr<freeusd::sdf::Layer>& L : compose_) {
      if (!L) {
        continue;
      }
      for (const double t : L->ListSampleTimes(prim_path, name)) {
        times.insert(t);
      }
    }
  }
  return std::vector<double>(times.begin(), times.end());
}

std::vector<std::string> Stage::ListComposedRelationshipNames(const freeusd::sdf::Path& prim_path) const {
  std::set<std::string> keys;
  for (const std::shared_ptr<freeusd::sdf::Layer>& L : compose_) {
    if (!L) {
      continue;
    }
    for (const std::string& k : L->ListRelationshipNames(prim_path)) {
      keys.insert(k);
    }
  }
  return std::vector<std::string>(keys.begin(), keys.end());
}

std::vector<freeusd::sdf::Path> Stage::ListComposedPrimPaths() const {
  std::set<std::string> sorted;
  for (const std::shared_ptr<freeusd::sdf::Layer>& L : compose_) {
    if (!L) {
      continue;
    }
    for (const freeusd::sdf::Path& p : L->ListPrimPaths()) {
      sorted.insert(p.GetString());
    }
  }
  std::vector<freeusd::sdf::Path> out;
  out.reserve(sorted.size());
  for (const std::string& s : sorted) {
    out.push_back(freeusd::sdf::Path::FromString(s));
  }
  return out;
}

bool Stage::GetComposedRelocateTarget(const freeusd::sdf::Path& fromPrimPath, freeusd::sdf::Path* outToPrimPath) const {
  if (!outToPrimPath || !fromPrimPath.IsPrimPath()) {
    return false;
  }
  for (const std::shared_ptr<freeusd::sdf::Layer>& L : compose_) {
    if (L && L->HasRelocate(fromPrimPath)) {
      return L->GetRelocateTarget(fromPrimPath, outToPrimPath);
    }
  }
  return false;
}

bool Stage::RelocateSourceInAnyLayer(const freeusd::sdf::Path& fromPrimPath) const {
  if (!fromPrimPath.IsPrimPath()) {
    return false;
  }
  for (const std::shared_ptr<freeusd::sdf::Layer>& L : compose_) {
    if (L && L->HasRelocate(fromPrimPath)) {
      return true;
    }
  }
  return false;
}

std::vector<std::pair<freeusd::sdf::Path, freeusd::sdf::Path>> Stage::ListComposedRelocates() const {
  std::unordered_set<std::string> seen_from;
  std::vector<std::pair<freeusd::sdf::Path, freeusd::sdf::Path>> acc;
  for (const std::shared_ptr<freeusd::sdf::Layer>& L : compose_) {
    if (!L) {
      continue;
    }
    for (const auto& pr : L->ListRelocates()) {
      const std::string& fs = pr.first.GetString();
      if (seen_from.insert(fs).second) {
        acc.emplace_back(pr.first, pr.second);
      }
    }
  }
  std::sort(acc.begin(), acc.end(), [](const std::pair<freeusd::sdf::Path, freeusd::sdf::Path>& a,
                                        const std::pair<freeusd::sdf::Path, freeusd::sdf::Path>& b) {
    return a.first.GetString() < b.first.GetString();
  });
  return acc;
}

bool Stage::GetComposedPrefixSubstitution(const std::string& fromPrefix, std::string* outToPrefix) const {
  if (!outToPrefix || fromPrefix.empty()) {
    return false;
  }
  for (const std::shared_ptr<freeusd::sdf::Layer>& L : compose_) {
    if (L && L->HasPrefixSubstitution(fromPrefix)) {
      return L->GetPrefixSubstitution(fromPrefix, outToPrefix);
    }
  }
  return false;
}

bool Stage::PrefixSubstitutionKeyInAnyLayer(const std::string& fromPrefix) const {
  if (fromPrefix.empty()) {
    return false;
  }
  for (const std::shared_ptr<freeusd::sdf::Layer>& L : compose_) {
    if (L && L->HasPrefixSubstitution(fromPrefix)) {
      return true;
    }
  }
  return false;
}

std::vector<std::pair<std::string, std::string>> Stage::ListComposedPrefixSubstitutions() const {
  std::unordered_set<std::string> seen_from;
  std::vector<std::pair<std::string, std::string>> acc;
  for (const std::shared_ptr<freeusd::sdf::Layer>& L : compose_) {
    if (!L) {
      continue;
    }
    for (const auto& pr : L->ListPrefixSubstitutions()) {
      if (seen_from.insert(pr.first).second) {
        acc.emplace_back(pr.first, pr.second);
      }
    }
  }
  std::sort(acc.begin(), acc.end(), [](const std::pair<std::string, std::string>& a,
                                        const std::pair<std::string, std::string>& b) {
    return a.first < b.first;
  });
  return acc;
}

bool Stage::HasDefaultPrim() const {
  const std::shared_ptr<freeusd::sdf::Layer> r = GetRootLayerPtr();
  return static_cast<bool>(r && r->HasDefaultPrim());
}

std::string Stage::GetDefaultPrimName() const {
  const std::shared_ptr<freeusd::sdf::Layer> r = GetRootLayerPtr();
  if (!r || !r->HasDefaultPrim()) {
    return {};
  }
  const std::optional<std::string_view> dp = r->GetDefaultPrim();
  if (!dp.has_value()) {
    return {};
  }
  return std::string{*dp};
}

Prim Stage::GetDefaultPrim() const {
  if (!HasDefaultPrim()) {
    return {};
  }
  const std::string n = GetDefaultPrimName();
  if (n.empty()) {
    return {};
  }
  const freeusd::sdf::Path p =
      freeusd::sdf::Path::AbsoluteRootPath().AppendChild(freeusd::tf::Token(n));
  return GetPrimAtPath(p);
}

std::optional<double> Stage::GetStartTimeCode() const {
  for (const std::shared_ptr<freeusd::sdf::Layer>& L : compose_) {
    if (!L) {
      continue;
    }
    const std::optional<double> v = L->GetStartTimeCode();
    if (v.has_value()) {
      return v;
    }
  }
  return std::nullopt;
}

std::optional<double> Stage::GetEndTimeCode() const {
  for (const std::shared_ptr<freeusd::sdf::Layer>& L : compose_) {
    if (!L) {
      continue;
    }
    const std::optional<double> v = L->GetEndTimeCode();
    if (v.has_value()) {
      return v;
    }
  }
  return std::nullopt;
}

std::optional<double> Stage::GetTimeCodesPerSecond() const {
  for (const std::shared_ptr<freeusd::sdf::Layer>& L : compose_) {
    if (!L) {
      continue;
    }
    const std::optional<double> v = L->GetTimeCodesPerSecond();
    if (v.has_value()) {
      return v;
    }
  }
  return std::nullopt;
}

std::optional<double> Stage::GetFramesPerSecond() const {
  for (const std::shared_ptr<freeusd::sdf::Layer>& L : compose_) {
    if (!L) {
      continue;
    }
    const std::optional<double> v = L->GetFramesPerSecond();
    if (v.has_value()) {
      return v;
    }
  }
  return std::nullopt;
}

std::optional<int> Stage::GetFramePrecision() const {
  for (const std::shared_ptr<freeusd::sdf::Layer>& L : compose_) {
    if (!L) {
      continue;
    }
    const std::optional<int> v = L->GetFramePrecision();
    if (v.has_value()) {
      return v;
    }
  }
  return std::nullopt;
}

std::optional<double> Stage::GetMetersPerUnit() const {
  for (const std::shared_ptr<freeusd::sdf::Layer>& L : compose_) {
    if (!L) {
      continue;
    }
    const std::optional<double> v = L->GetMetersPerUnit();
    if (v.has_value()) {
      return v;
    }
  }
  return std::nullopt;
}

std::optional<std::string> Stage::GetUpAxis() const {
  for (const std::shared_ptr<freeusd::sdf::Layer>& L : compose_) {
    if (!L) {
      continue;
    }
    const std::optional<std::string> v = L->GetUpAxis();
    if (v.has_value()) {
      return v;
    }
  }
  return std::nullopt;
}

std::vector<freeusd::sdf::Path> Stage::GetPrimOrder() const {
  for (const std::shared_ptr<freeusd::sdf::Layer>& L : compose_) {
    if (!L) {
      continue;
    }
    const std::vector<freeusd::sdf::Path>& po = L->GetPrimOrder();
    if (!po.empty()) {
      return po;
    }
  }
  return {};
}

freeusd::sdf::Path Stage::GetPseudoRootPath() const { return freeusd::sdf::Path::AbsoluteRootPath(); }

Prim Stage::GetPrimAtPath(const freeusd::sdf::Path& path) const {
  if (!path.IsPrimPath()) {
    return {};
  }
  return Prim{weak_from_this(), path};
}

void Stage::TraversePreorder(const std::function<bool(const Prim& prim)>& visitor) const {
  if (!visitor) {
    return;
  }
  const std::weak_ptr<const Stage> wp = weak_from_this();
  const std::function<void(const freeusd::sdf::Path&)> descend = [&](const freeusd::sdf::Path& path) {
    const Prim prim{wp, path};
    if (!prim.IsValid()) {
      return;
    }
    if (!visitor(prim)) {
      return;
    }
    for (const Prim& ch : GetChildren(path)) {
      descend(ch.GetPath());
    }
  };
  for (const Prim& root : GetChildren(freeusd::sdf::Path::AbsoluteRootPath())) {
    descend(root.GetPath());
  }
}

std::vector<Prim> Stage::GetChildren(const freeusd::sdf::Path& primPath) const {
  std::vector<Prim> out;
  if (!primPath.IsPrimPath() && !primPath.IsAbsoluteRootPath()) {
    return out;
  }
  std::unordered_map<std::string, freeusd::sdf::Path> unique;
  for (const std::shared_ptr<freeusd::sdf::Layer>& L : compose_) {
    if (!L) {
      continue;
    }
    for (const auto& p : L->ListPrimPaths()) {
      if (!p.IsPrimPath()) {
        continue;
      }
      if (p.GetParentPath() == primPath) {
        unique[p.GetString()] = p;
      }
    }
  }
  out.reserve(unique.size());
  for (const auto& e : unique) {
    out.emplace_back(weak_from_this(), e.second);
  }
  std::sort(out.begin(), out.end(), [](const Prim& a, const Prim& b) {
    return a.GetPath().GetString() < b.GetPath().GetString();
  });
  return out;
}

void Stage::SetResolver(std::unique_ptr<freeusd::ar::Resolver> resolver) {
  if (resolver) {
    resolver_ = std::move(resolver);
  }
}

}  // namespace freeusd::usd
