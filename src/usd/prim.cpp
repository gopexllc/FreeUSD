#include "freeusd/usd/prim.hpp"

#include "freeusd/usd/stage.hpp"

namespace freeusd::usd {

Prim::Prim(std::weak_ptr<const Stage> stage, freeusd::sdf::Path path) : stage_(std::move(stage)), path_(std::move(path)) {}

std::shared_ptr<const Stage> Prim::lock_stage() const { return stage_.lock(); }

bool Prim::IsValid() const noexcept {
  const auto st = lock_stage();
  if (!st || !path_.IsPrimPath()) {
    return false;
  }
  return st->PrimPathInUse(path_);
}

bool Prim::HasAttribute(const freeusd::tf::Token& name) const {
  const auto st = lock_stage();
  if (!st || name.IsEmpty()) {
    return false;
  }
  return st->HasFieldOpinion(path_, name);
}

freeusd::vt::Value Prim::GetAttribute(const freeusd::tf::Token& name) const {
  const auto st = lock_stage();
  freeusd::vt::Value v;
  if (!st || name.IsEmpty()) {
    return v;
  }
  constexpr double kDefaultStageTime = 1.0;
  st->ReadFieldAtEvaluatedTime(path_, name, kDefaultStageTime, &v);
  return v;
}

bool Prim::HasRelationship(const freeusd::tf::Token& relName) const {
  const auto st = lock_stage();
  if (!st || relName.IsEmpty()) {
    return false;
  }
  for (const std::shared_ptr<freeusd::sdf::Layer>& L : st->GetComposeLayers()) {
    if (L && L->HasRelationship(path_, relName)) {
      return true;
    }
  }
  return false;
}

std::vector<freeusd::sdf::Path> Prim::GetRelationshipTargets(const freeusd::tf::Token& relName) const {
  const auto st = lock_stage();
  std::vector<freeusd::sdf::Path> out;
  if (!st || relName.IsEmpty()) {
    return out;
  }
  st->ReadRelationship(path_, relName, &out);
  return out;
}

freeusd::tf::Token Prim::GetPrimKind() const {
  const auto st = lock_stage();
  if (!st) {
    return {};
  }
  return st->ResolvePrimKind(path_);
}

bool Prim::HasPrimKind() const {
  const auto st = lock_stage();
  if (!st) {
    return false;
  }
  return st->ResolveHasPrimKind(path_);
}

bool Prim::IsActive() const {
  const auto st = lock_stage();
  if (!st) {
    return true;
  }
  return st->ResolvePrimActive(path_);
}

bool Prim::HasPrimActiveOpinion() const {
  const auto st = lock_stage();
  if (!st) {
    return false;
  }
  return st->ResolveHasPrimActiveOpinion(path_);
}

bool Prim::HasCustomDataKey(const std::string& key) const {
  const auto st = lock_stage();
  if (!st || key.empty()) {
    return false;
  }
  return st->PrimCustomDataKeyInAnyLayer(path_, key);
}

freeusd::vt::Value Prim::GetCustomData(const std::string& key) const {
  const auto st = lock_stage();
  freeusd::vt::Value v;
  if (!st || key.empty()) {
    return v;
  }
  st->GetComposedPrimCustomData(path_, key, &v);
  return v;
}

std::vector<std::string> Prim::ListCustomDataKeys() const {
  const auto st = lock_stage();
  if (!st) {
    return {};
  }
  return st->ListComposedPrimCustomDataKeys(path_);
}

}  // namespace freeusd::usd
