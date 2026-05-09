#include "freeusd/usd/stage.hpp"

#include <algorithm>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "freeusd/pcp/resolve.hpp"
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

freeusd::sdf::Path Stage::GetPseudoRootPath() const { return freeusd::sdf::Path::AbsoluteRootPath(); }

Prim Stage::GetPrimAtPath(const freeusd::sdf::Path& path) const {
  if (!path.IsPrimPath()) {
    return {};
  }
  return Prim{weak_from_this(), path};
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
