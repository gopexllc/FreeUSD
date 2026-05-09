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

std::string Prim::GetName() const {
  if (!path_.IsPrimPath()) {
    return {};
  }
  return path_.GetName();
}

std::shared_ptr<const Stage> Prim::GetStage() const { return lock_stage(); }

Prim Prim::GetParent() const {
  const auto st = lock_stage();
  if (!st || !path_.IsPrimPath() || path_.IsAbsoluteRootPath()) {
    return {};
  }
  const freeusd::sdf::Path par = path_.GetParentPath();
  if (par.IsAbsoluteRootPath()) {
    return {};
  }
  return Prim{stage_, par};
}

std::vector<Prim> Prim::GetChildren() const {
  const auto st = lock_stage();
  if (!st || !IsValid()) {
    return {};
  }
  return st->GetChildren(path_);
}

bool Prim::HasAttribute(const freeusd::tf::Token& name) const {
  const auto st = lock_stage();
  if (!st || name.IsEmpty()) {
    return false;
  }
  return st->HasFieldOpinion(path_, name);
}

freeusd::vt::Value Prim::GetAttribute(const freeusd::tf::Token& name, double time) const {
  const auto st = lock_stage();
  freeusd::vt::Value v;
  if (!st || name.IsEmpty()) {
    return v;
  }
  st->ReadFieldAtEvaluatedTime(path_, name, time, &v);
  return v;
}

std::vector<std::string> Prim::ListAttributeNames() const {
  const auto st = lock_stage();
  if (!st) {
    return {};
  }
  return st->ListComposedFieldNames(path_);
}

std::vector<std::string> Prim::ListRelationshipNames() const {
  const auto st = lock_stage();
  if (!st) {
    return {};
  }
  return st->ListComposedRelationshipNames(path_);
}

std::vector<double> Prim::ListAttributeSampleTimes(const freeusd::tf::Token& name) const {
  const auto st = lock_stage();
  if (!st) {
    return {};
  }
  return st->ListComposedFieldSampleTimes(path_, name);
}

bool Prim::HasAttributeConnection(const freeusd::tf::Token& attr_name) const {
  const auto st = lock_stage();
  if (!st || attr_name.IsEmpty()) {
    return false;
  }
  return st->HasAttributeConnection(path_, attr_name);
}

bool Prim::GetAttributeConnectionTarget(const freeusd::tf::Token& attr_name, freeusd::sdf::Path* out_target) const {
  const auto st = lock_stage();
  if (!st || !out_target || attr_name.IsEmpty()) {
    return false;
  }
  return st->GetComposedAttributeConnectionTarget(path_, attr_name, out_target);
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

std::vector<freeusd::sdf::PrimReference> Prim::GetReferences() const {
  const auto st = lock_stage();
  if (!st) {
    return {};
  }
  return st->ReadPrimReferences(path_);
}

bool Prim::HasReferences() const {
  const auto st = lock_stage();
  if (!st) {
    return false;
  }
  return st->HasPrimReferences(path_);
}

std::vector<freeusd::sdf::Path> Prim::GetInherits() const {
  const auto st = lock_stage();
  if (!st) {
    return {};
  }
  return st->ReadPrimInherits(path_);
}

bool Prim::HasInherits() const {
  const auto st = lock_stage();
  if (!st) {
    return false;
  }
  return st->HasPrimInherits(path_);
}

std::vector<freeusd::sdf::Path> Prim::GetSpecializes() const {
  const auto st = lock_stage();
  if (!st) {
    return {};
  }
  return st->ReadPrimSpecializes(path_);
}

bool Prim::HasSpecializes() const {
  const auto st = lock_stage();
  if (!st) {
    return false;
  }
  return st->HasPrimSpecializes(path_);
}

std::vector<freeusd::sdf::PrimReference> Prim::GetPayloads() const {
  const auto st = lock_stage();
  if (!st) {
    return {};
  }
  return st->ReadPrimPayloads(path_);
}

bool Prim::HasPayloads() const {
  const auto st = lock_stage();
  if (!st) {
    return false;
  }
  return st->HasPrimPayloads(path_);
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

freeusd::sdf::Layer::PrimSpecifierKind Prim::GetSpecifierKind() const {
  const auto st = lock_stage();
  if (!st) {
    return freeusd::sdf::Layer::PrimSpecifierKind::Default;
  }
  return st->ResolvePrimSpecifierKind(path_);
}

bool Prim::IsAbstract() const {
  return GetSpecifierKind() == freeusd::sdf::Layer::PrimSpecifierKind::Class;
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

bool Prim::HasVariantSelectionKey(const std::string& variantSet) const {
  const auto st = lock_stage();
  if (!st || variantSet.empty()) {
    return false;
  }
  return st->PrimVariantSelectionSetInAnyLayer(path_, variantSet);
}

std::string Prim::GetVariantSelection(const std::string& variantSet) const {
  const auto st = lock_stage();
  if (!st || variantSet.empty()) {
    return {};
  }
  std::string name;
  if (st->GetComposedPrimVariantSelection(path_, variantSet, &name)) {
    return name;
  }
  return {};
}

std::vector<std::string> Prim::ListVariantSelectionSets() const {
  const auto st = lock_stage();
  if (!st) {
    return {};
  }
  return st->ListComposedPrimVariantSelectionSets(path_);
}

bool Prim::HasVariantSet(const std::string& variantSetName) const {
  const auto st = lock_stage();
  if (!st || variantSetName.empty()) {
    return false;
  }
  return st->PrimVariantSetDeclaredInAnyLayer(path_, variantSetName);
}

std::vector<std::string> Prim::ListVariantSetNames() const {
  const auto st = lock_stage();
  if (!st) {
    return {};
  }
  return st->ListComposedPrimVariantSetNames(path_);
}

std::vector<std::string> Prim::ListVariantNames(const std::string& variantSetName) const {
  const auto st = lock_stage();
  if (!st) {
    return {};
  }
  return st->GetComposedPrimVariantNames(path_, variantSetName);
}

}  // namespace freeusd::usd
