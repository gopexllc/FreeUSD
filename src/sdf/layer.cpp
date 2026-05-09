#include "freeusd/sdf/layer.hpp"

#include <algorithm>
#include <cctype>
#include <utility>

namespace {
std::string_view trim_sv(std::string_view sv) {
  while (!sv.empty() && std::isspace(static_cast<unsigned char>(sv.front()))) {
    sv.remove_prefix(1);
  }
  while (!sv.empty() && std::isspace(static_cast<unsigned char>(sv.back()))) {
    sv.remove_suffix(1);
  }
  return sv;
}
}  // namespace

namespace freeusd::sdf {

Layer::Layer(std::string identifier) : identifier_(std::move(identifier)) {}

std::shared_ptr<Layer> Layer::NewAnonymous(std::string identifier) {
  return std::shared_ptr<Layer>(new Layer(std::move(identifier)));
}

void Layer::touch_hierarchy(const Path& primPath) {
  for (Path p = primPath; !p.IsEmpty() && p.IsAbsolutePath(); p = p.GetParentPath()) {
    hierarchy_.insert(p);
    if (p.IsAbsoluteRootPath()) {
      break;
    }
  }
}

FieldOpinion* Layer::find_opinion(const Path& primPath, const std::string& name) {
  auto it = fields_.find(primPath);
  if (it == fields_.end()) {
    return nullptr;
  }
  auto jt = it->second.find(name);
  if (jt == it->second.end()) {
    return nullptr;
  }
  return &jt->second;
}

const FieldOpinion* Layer::find_opinion(const Path& primPath, const std::string& name) const {
  auto it = fields_.find(primPath);
  if (it == fields_.end()) {
    return nullptr;
  }
  auto jt = it->second.find(name);
  if (jt == it->second.end()) {
    return nullptr;
  }
  return &jt->second;
}

void Layer::SetField(const Path& primPath, const freeusd::tf::Token& name, const freeusd::vt::Value& value) {
  if (!primPath.IsPrimPath()) {
    return;
  }
  if (name.IsEmpty()) {
    return;
  }
  touch_hierarchy(primPath);
  fields_[primPath][name.GetText()].SetDefault(value);
}

bool Layer::HasField(const Path& primPath, const freeusd::tf::Token& name) const {
  if (name.IsEmpty()) {
    return false;
  }
  const auto* op = find_opinion(primPath, name.GetText());
  return op && op->HasAny();
}

std::vector<std::string> Layer::ListFieldNames(const Path& primPath) const {
  std::vector<std::string> out;
  const auto it = fields_.find(primPath);
  if (it == fields_.end()) {
    return out;
  }
  for (const auto& e : it->second) {
    if (e.second.HasAny()) {
      out.push_back(e.first);
    }
  }
  std::sort(out.begin(), out.end());
  return out;
}

bool Layer::GetField(const Path& primPath, const freeusd::tf::Token& name, freeusd::vt::Value* out) const {
  if (!out || name.IsEmpty()) {
    return false;
  }
  const auto* op = find_opinion(primPath, name.GetText());
  if (!op || !op->default_value) {
    return false;
  }
  *out = *op->default_value;
  return true;
}

bool Layer::GetFieldAtTime(const Path& primPath, const freeusd::tf::Token& name, double time, freeusd::vt::Value* out) const {
  if (!out || name.IsEmpty()) {
    return false;
  }
  const auto* op = find_opinion(primPath, name.GetText());
  if (!op) {
    return false;
  }
  return op->EvaluateAt(time, out);
}

void Layer::SetTimeSample(const Path& primPath, const freeusd::tf::Token& name, double time, const freeusd::vt::Value& value) {
  if (!primPath.IsPrimPath() || name.IsEmpty()) {
    return;
  }
  touch_hierarchy(primPath);
  fields_[primPath][name.GetText()].SetSample(time, value);
}

std::vector<double> Layer::ListSampleTimes(const Path& primPath, const freeusd::tf::Token& name) const {
  const auto* op = find_opinion(primPath, name.GetText());
  if (!op) {
    return {};
  }
  return op->ListTimes();
}

bool Layer::GetTimeSample(const Path& primPath, const freeusd::tf::Token& name, double time, freeusd::vt::Value* out) const {
  const auto* op = find_opinion(primPath, name.GetText());
  if (!op) {
    return false;
  }
  return op->GetExactSample(time, out);
}

void Layer::ClearTimeSamples(const Path& primPath, const freeusd::tf::Token& name) {
  auto* op = find_opinion(primPath, name.GetText());
  if (!op) {
    return;
  }
  op->ClearSamples();
  if (!op->HasAny()) {
    auto it = fields_.find(primPath);
    if (it != fields_.end()) {
      it->second.erase(name.GetText());
    }
  }
}

std::vector<Path> Layer::ListPrimPaths() const {
  std::vector<Path> out;
  out.reserve(hierarchy_.size());
  for (const auto& p : hierarchy_) {
    out.push_back(p);
  }
  std::sort(out.begin(), out.end(), [](const Path& a, const Path& b) { return a.GetString() < b.GetString(); });
  return out;
}

void Layer::Clear() noexcept {
  fields_.clear();
  hierarchy_.clear();
  prim_kinds_.clear();
  documentation_.clear();
  default_prim_.reset();
  sublayer_paths_.clear();
  relocates_.clear();
  references_.clear();
  prim_inherits_.clear();
  prim_specializes_.clear();
  prim_payloads_.clear();
  relationships_.clear();
  prim_active_.clear();
  prim_specifiers_.clear();
  prim_custom_data_.clear();
  prim_variant_selection_.clear();
  prim_variant_sets_.clear();
  attribute_connections_.clear();
}

void Layer::SetDefaultPrim(std::string_view rootPrimName) {
  const std::string_view t = trim_sv(rootPrimName);
  if (t.empty()) {
    default_prim_.reset();
    return;
  }
  default_prim_ = std::string{t};
}

void Layer::SetSubLayers(std::vector<std::string> paths) {
  sublayer_paths_.clear();
  sublayer_paths_.reserve(paths.size());
  for (auto& p : paths) {
    std::string_view v = trim_sv(p);
    if (!v.empty()) {
      sublayer_paths_.emplace_back(v);
    }
  }
}

void Layer::ClearRelocates() noexcept {
  relocates_.clear();
}

void Layer::SetRelocate(Path fromPrimPath, Path toPrimPath) {
  if (!fromPrimPath.IsPrimPath() || !toPrimPath.IsPrimPath()) {
    return;
  }
  if (fromPrimPath.IsEmpty() || toPrimPath.IsEmpty()) {
    return;
  }
  relocates_[std::move(fromPrimPath)] = std::move(toPrimPath);
}

void Layer::EraseRelocate(const Path& fromPrimPath) {
  relocates_.erase(fromPrimPath);
}

bool Layer::HasRelocate(const Path& fromPrimPath) const {
  return relocates_.find(fromPrimPath) != relocates_.end();
}

bool Layer::GetRelocateTarget(const Path& fromPrimPath, Path* outToPrimPath) const {
  if (!outToPrimPath) {
    return false;
  }
  const auto it = relocates_.find(fromPrimPath);
  if (it == relocates_.end()) {
    return false;
  }
  *outToPrimPath = it->second;
  return true;
}

std::vector<std::pair<Path, Path>> Layer::ListRelocates() const {
  std::vector<std::pair<Path, Path>> out;
  out.reserve(relocates_.size());
  for (const auto& e : relocates_) {
    out.emplace_back(e.first, e.second);
  }
  std::sort(out.begin(), out.end(),
            [](const std::pair<Path, Path>& a, const std::pair<Path, Path>& b) {
              return a.first.GetString() < b.first.GetString();
            });
  return out;
}

void Layer::AddPrimReference(const Path& primPath, PrimReference ref) {
  if (!primPath.IsPrimPath() || ref.IsEmpty()) {
    return;
  }
  touch_hierarchy(primPath);
  references_[primPath].push_back(std::move(ref));
}

void Layer::AddReference(const Path& primPath, std::string authoredFragment) {
  PrimReference r;
  if (!PrimReference::ParseAuthored(authoredFragment, &r)) {
    return;
  }
  AddPrimReference(primPath, std::move(r));
}

void Layer::SetPrimReferences(const Path& primPath, std::vector<PrimReference> refs) {
  references_.erase(primPath);
  for (auto& r : refs) {
    AddPrimReference(primPath, std::move(r));
  }
}

void Layer::SetReferences(const Path& primPath, std::vector<std::string> authoredFragments) {
  references_.erase(primPath);
  for (auto& s : authoredFragments) {
    AddReference(primPath, std::move(s));
  }
}

std::vector<PrimReference> Layer::ListPrimReferences(const Path& primPath) const {
  const auto it = references_.find(primPath);
  if (it == references_.end()) {
    return {};
  }
  return it->second;
}

std::vector<std::string> Layer::ListReferences(const Path& primPath) const {
  const auto it = references_.find(primPath);
  if (it == references_.end()) {
    return {};
  }
  std::vector<std::string> out;
  out.reserve(it->second.size());
  for (const PrimReference& r : it->second) {
    out.push_back(r.FormatAuthoredForUsda());
  }
  return out;
}

void Layer::ClearReferences(const Path& primPath) { references_.erase(primPath); }

void Layer::ClearPrimInherits(const Path& primPath) { prim_inherits_.erase(primPath); }

void Layer::AddPrimInherit(const Path& primPath, Path targetPrimPath) {
  if (!primPath.IsPrimPath() || !targetPrimPath.IsPrimPath() || targetPrimPath.IsEmpty()) {
    return;
  }
  touch_hierarchy(primPath);
  prim_inherits_[primPath].push_back(std::move(targetPrimPath));
}

void Layer::PrependPrimInherits(const Path& primPath, std::vector<Path> front) {
  if (!primPath.IsPrimPath() || front.empty()) {
    return;
  }
  touch_hierarchy(primPath);
  auto& v = prim_inherits_[primPath];
  v.insert(v.begin(), front.begin(), front.end());
}

void Layer::AppendPrimInherits(const Path& primPath, std::vector<Path> back) {
  if (!primPath.IsPrimPath() || back.empty()) {
    return;
  }
  touch_hierarchy(primPath);
  auto& v = prim_inherits_[primPath];
  v.insert(v.end(), back.begin(), back.end());
}

void Layer::SetPrimInherits(const Path& primPath, std::vector<Path> targets) {
  ClearPrimInherits(primPath);
  for (Path& t : targets) {
    AddPrimInherit(primPath, std::move(t));
  }
}

std::vector<Path> Layer::ListPrimInherits(const Path& primPath) const {
  const auto it = prim_inherits_.find(primPath);
  if (it == prim_inherits_.end()) {
    return {};
  }
  return it->second;
}

void Layer::ClearPrimSpecializes(const Path& primPath) { prim_specializes_.erase(primPath); }

void Layer::AddPrimSpecializes(const Path& primPath, Path targetPrimPath) {
  if (!primPath.IsPrimPath() || !targetPrimPath.IsPrimPath() || targetPrimPath.IsEmpty()) {
    return;
  }
  touch_hierarchy(primPath);
  prim_specializes_[primPath].push_back(std::move(targetPrimPath));
}

void Layer::PrependPrimSpecializes(const Path& primPath, std::vector<Path> front) {
  if (!primPath.IsPrimPath() || front.empty()) {
    return;
  }
  touch_hierarchy(primPath);
  auto& v = prim_specializes_[primPath];
  v.insert(v.begin(), front.begin(), front.end());
}

void Layer::AppendPrimSpecializes(const Path& primPath, std::vector<Path> back) {
  if (!primPath.IsPrimPath() || back.empty()) {
    return;
  }
  touch_hierarchy(primPath);
  auto& v = prim_specializes_[primPath];
  v.insert(v.end(), back.begin(), back.end());
}

void Layer::SetPrimSpecializes(const Path& primPath, std::vector<Path> targets) {
  ClearPrimSpecializes(primPath);
  for (Path& t : targets) {
    AddPrimSpecializes(primPath, std::move(t));
  }
}

std::vector<Path> Layer::ListPrimSpecializes(const Path& primPath) const {
  const auto it = prim_specializes_.find(primPath);
  if (it == prim_specializes_.end()) {
    return {};
  }
  return it->second;
}

void Layer::ClearPrimPayloads(const Path& primPath) { prim_payloads_.erase(primPath); }

void Layer::AddPrimPayload(const Path& primPath, PrimReference ref) {
  if (!primPath.IsPrimPath() || ref.IsEmpty()) {
    return;
  }
  touch_hierarchy(primPath);
  prim_payloads_[primPath].push_back(std::move(ref));
}

void Layer::SetPrimPayloads(const Path& primPath, std::vector<PrimReference> refs) {
  prim_payloads_.erase(primPath);
  for (auto& r : refs) {
    AddPrimPayload(primPath, std::move(r));
  }
}

std::vector<PrimReference> Layer::ListPrimPayloads(const Path& primPath) const {
  const auto it = prim_payloads_.find(primPath);
  if (it == prim_payloads_.end()) {
    return {};
  }
  return it->second;
}

std::vector<std::string> Layer::ListPayloads(const Path& primPath) const {
  const auto it = prim_payloads_.find(primPath);
  if (it == prim_payloads_.end()) {
    return {};
  }
  std::vector<std::string> out;
  out.reserve(it->second.size());
  for (const PrimReference& r : it->second) {
    out.push_back(r.FormatAuthoredForUsda());
  }
  return out;
}

void Layer::SetRelationshipTargets(const Path& primPath, const freeusd::tf::Token& relName,
                                  std::vector<Path> targets) {
  if (!primPath.IsPrimPath() || relName.IsEmpty()) {
    return;
  }
  touch_hierarchy(primPath);
  auto& book = relationships_[primPath];
  if (targets.empty()) {
    book.erase(relName.GetText());
    if (book.empty()) {
      relationships_.erase(primPath);
    }
    return;
  }
  book[relName.GetText()] = std::move(targets);
}

void Layer::PrependRelationshipTargets(const Path& primPath, const freeusd::tf::Token& relName,
                                       std::vector<Path> extraFront) {
  if (!primPath.IsPrimPath() || relName.IsEmpty() || extraFront.empty()) {
    return;
  }
  touch_hierarchy(primPath);
  auto& vec = relationships_[primPath][relName.GetText()];
  vec.insert(vec.begin(), extraFront.begin(), extraFront.end());
}

void Layer::AppendRelationshipTargets(const Path& primPath, const freeusd::tf::Token& relName,
                                        std::vector<Path> extraBack) {
  if (!primPath.IsPrimPath() || relName.IsEmpty() || extraBack.empty()) {
    return;
  }
  touch_hierarchy(primPath);
  auto& vec = relationships_[primPath][relName.GetText()];
  vec.insert(vec.end(), extraBack.begin(), extraBack.end());
}

void Layer::DeleteRelationshipTargets(const Path& primPath, const freeusd::tf::Token& relName,
                                       std::vector<Path> toRemove) {
  if (!primPath.IsPrimPath() || relName.IsEmpty() || toRemove.empty()) {
    return;
  }
  touch_hierarchy(primPath);
  auto it = relationships_.find(primPath);
  if (it == relationships_.end()) {
    return;
  }
  auto jt = it->second.find(relName.GetText());
  if (jt == it->second.end()) {
    return;
  }
  std::vector<Path>& vec = jt->second;
  for (const Path& kill : toRemove) {
    vec.erase(std::remove(vec.begin(), vec.end(), kill), vec.end());
  }
  if (vec.empty()) {
    it->second.erase(relName.GetText());
    if (it->second.empty()) {
      relationships_.erase(it);
    }
  }
}

void Layer::ClearRelationship(const Path& primPath, const freeusd::tf::Token& relName) {
  if (!primPath.IsPrimPath() || relName.IsEmpty()) {
    return;
  }
  auto it = relationships_.find(primPath);
  if (it == relationships_.end()) {
    return;
  }
  it->second.erase(relName.GetText());
  if (it->second.empty()) {
    relationships_.erase(it);
  }
}

bool Layer::HasRelationship(const Path& primPath, const freeusd::tf::Token& relName) const {
  if (!primPath.IsPrimPath() || relName.IsEmpty()) {
    return false;
  }
  const auto it = relationships_.find(primPath);
  if (it == relationships_.end()) {
    return false;
  }
  return it->second.find(relName.GetText()) != it->second.end();
}

std::vector<Path> Layer::GetRelationshipTargets(const Path& primPath, const freeusd::tf::Token& relName) const {
  if (!primPath.IsPrimPath() || relName.IsEmpty()) {
    return {};
  }
  const auto it = relationships_.find(primPath);
  if (it == relationships_.end()) {
    return {};
  }
  const auto jt = it->second.find(relName.GetText());
  if (jt == it->second.end()) {
    return {};
  }
  return jt->second;
}

std::vector<std::string> Layer::ListRelationshipNames(const Path& primPath) const {
  std::vector<std::string> out;
  const auto it = relationships_.find(primPath);
  if (it == relationships_.end()) {
    return out;
  }
  out.reserve(it->second.size());
  for (const auto& e : it->second) {
    out.push_back(e.first);
  }
  std::sort(out.begin(), out.end());
  return out;
}

void Layer::SetAttributeConnection(const Path& primPath, const freeusd::tf::Token& name, Path targetProp) {
  if (!primPath.IsPrimPath() || name.IsEmpty() || !targetProp.IsPropertyPath()) {
    return;
  }
  touch_hierarchy(primPath);
  attribute_connections_[primPath][name.GetText()] = std::move(targetProp);
}

void Layer::ClearAttributeConnection(const Path& primPath, const freeusd::tf::Token& name) {
  if (!primPath.IsPrimPath() || name.IsEmpty()) {
    return;
  }
  auto it = attribute_connections_.find(primPath);
  if (it == attribute_connections_.end()) {
    return;
  }
  it->second.erase(name.GetText());
  if (it->second.empty()) {
    attribute_connections_.erase(it);
  }
}

bool Layer::HasAttributeConnection(const Path& primPath, const freeusd::tf::Token& name) const {
  if (!primPath.IsPrimPath() || name.IsEmpty()) {
    return false;
  }
  const auto it = attribute_connections_.find(primPath);
  if (it == attribute_connections_.end()) {
    return false;
  }
  return it->second.find(name.GetText()) != it->second.end();
}

bool Layer::GetAttributeConnectionTarget(const Path& primPath, const freeusd::tf::Token& name,
                                         Path* targetProp) const {
  if (!targetProp || !primPath.IsPrimPath() || name.IsEmpty()) {
    return false;
  }
  const auto it = attribute_connections_.find(primPath);
  if (it == attribute_connections_.end()) {
    return false;
  }
  const auto jt = it->second.find(name.GetText());
  if (jt == it->second.end()) {
    return false;
  }
  *targetProp = jt->second;
  return true;
}

std::vector<std::pair<std::string, Path>> Layer::ListAttributeConnections(const Path& primPath) const {
  std::vector<std::pair<std::string, Path>> out;
  if (!primPath.IsPrimPath()) {
    return out;
  }
  const auto it = attribute_connections_.find(primPath);
  if (it == attribute_connections_.end()) {
    return out;
  }
  out.reserve(it->second.size());
  for (const auto& e : it->second) {
    out.emplace_back(e.first, e.second);
  }
  std::sort(out.begin(), out.end(),
            [](const std::pair<std::string, Path>& a, const std::pair<std::string, Path>& b) {
              return a.first < b.first;
            });
  return out;
}

bool Layer::IsPrimActive(const Path& primPath) const noexcept {
  const auto it = prim_active_.find(primPath);
  if (it == prim_active_.end()) {
    return true;
  }
  return it->second;
}

void Layer::SetPrimActive(const Path& primPath, bool active) {
  if (!primPath.IsPrimPath()) {
    return;
  }
  touch_hierarchy(primPath);
  if (active) {
    prim_active_.erase(primPath);
  } else {
    prim_active_[primPath] = false;
  }
}

bool Layer::HasPrimActiveOpinion(const Path& primPath) const noexcept {
  return prim_active_.find(primPath) != prim_active_.end();
}

void Layer::SetPrimSpecifier(const Path& primPath, PrimSpecifierKind k) {
  if (!primPath.IsPrimPath()) {
    return;
  }
  touch_hierarchy(primPath);
  if (k == PrimSpecifierKind::Default || k == PrimSpecifierKind::Def) {
    prim_specifiers_.erase(primPath);
    return;
  }
  prim_specifiers_[primPath] = k;
}

Layer::PrimSpecifierKind Layer::GetPrimSpecifier(const Path& primPath) const noexcept {
  const auto it = prim_specifiers_.find(primPath);
  if (it == prim_specifiers_.end()) {
    return PrimSpecifierKind::Default;
  }
  return it->second;
}

void Layer::SetPrimKind(const Path& primPath, const freeusd::tf::Token& kind) {
  if (!primPath.IsPrimPath()) {
    return;
  }
  touch_hierarchy(primPath);
  if (kind.IsEmpty()) {
    prim_kinds_.erase(primPath);
    return;
  }
  prim_kinds_[primPath] = kind;
}

bool Layer::HasPrimKind(const Path& primPath) const {
  return prim_kinds_.find(primPath) != prim_kinds_.end();
}

freeusd::tf::Token Layer::GetPrimKind(const Path& primPath) const {
  const auto it = prim_kinds_.find(primPath);
  if (it == prim_kinds_.end()) {
    return freeusd::tf::Token{};
  }
  return it->second;
}

void Layer::ClearPrimCustomData(const Path& primPath) {
  prim_custom_data_.erase(primPath);
}

void Layer::SetPrimCustomDataEntry(const Path& primPath, std::string key, const freeusd::vt::Value& value) {
  if (!primPath.IsPrimPath() || key.empty() || value.IsEmpty()) {
    return;
  }
  touch_hierarchy(primPath);
  prim_custom_data_[primPath][std::move(key)] = value;
}

void Layer::ErasePrimCustomDataEntry(const Path& primPath, const std::string& key) {
  if (!primPath.IsPrimPath() || key.empty()) {
    return;
  }
  auto it = prim_custom_data_.find(primPath);
  if (it == prim_custom_data_.end()) {
    return;
  }
  it->second.erase(key);
  if (it->second.empty()) {
    prim_custom_data_.erase(it);
  }
}

bool Layer::HasPrimCustomDataKey(const Path& primPath, const std::string& key) const {
  if (!primPath.IsPrimPath() || key.empty()) {
    return false;
  }
  const auto it = prim_custom_data_.find(primPath);
  if (it == prim_custom_data_.end()) {
    return false;
  }
  return it->second.find(key) != it->second.end();
}

bool Layer::GetPrimCustomDataEntry(const Path& primPath, const std::string& key,
                                   freeusd::vt::Value* out) const {
  if (!out || !primPath.IsPrimPath() || key.empty()) {
    return false;
  }
  const auto it = prim_custom_data_.find(primPath);
  if (it == prim_custom_data_.end()) {
    return false;
  }
  const auto jt = it->second.find(key);
  if (jt == it->second.end()) {
    return false;
  }
  *out = jt->second;
  return true;
}

std::vector<std::string> Layer::ListPrimCustomDataKeys(const Path& primPath) const {
  std::vector<std::string> out;
  if (!primPath.IsPrimPath()) {
    return out;
  }
  const auto it = prim_custom_data_.find(primPath);
  if (it == prim_custom_data_.end()) {
    return out;
  }
  out.reserve(it->second.size());
  for (const auto& e : it->second) {
    out.push_back(e.first);
  }
  std::sort(out.begin(), out.end());
  return out;
}

void Layer::ClearPrimVariantSelection(const Path& primPath) {
  prim_variant_selection_.erase(primPath);
}

void Layer::SetPrimVariantSelectionEntry(const Path& primPath, std::string variantSet, std::string variantName) {
  if (!primPath.IsPrimPath() || variantSet.empty() || variantName.empty()) {
    return;
  }
  touch_hierarchy(primPath);
  prim_variant_selection_[primPath][std::move(variantSet)] = std::move(variantName);
}

void Layer::ErasePrimVariantSelectionEntry(const Path& primPath, const std::string& variantSet) {
  if (!primPath.IsPrimPath() || variantSet.empty()) {
    return;
  }
  auto it = prim_variant_selection_.find(primPath);
  if (it == prim_variant_selection_.end()) {
    return;
  }
  it->second.erase(variantSet);
  if (it->second.empty()) {
    prim_variant_selection_.erase(it);
  }
}

bool Layer::HasPrimVariantSelectionKey(const Path& primPath, const std::string& variantSet) const {
  if (!primPath.IsPrimPath() || variantSet.empty()) {
    return false;
  }
  const auto it = prim_variant_selection_.find(primPath);
  if (it == prim_variant_selection_.end()) {
    return false;
  }
  return it->second.find(variantSet) != it->second.end();
}

bool Layer::GetPrimVariantSelectionEntry(const Path& primPath, const std::string& variantSet,
                                         std::string* outName) const {
  if (!outName || !primPath.IsPrimPath() || variantSet.empty()) {
    return false;
  }
  const auto it = prim_variant_selection_.find(primPath);
  if (it == prim_variant_selection_.end()) {
    return false;
  }
  const auto jt = it->second.find(variantSet);
  if (jt == it->second.end()) {
    return false;
  }
  *outName = jt->second;
  return true;
}

std::vector<std::string> Layer::ListPrimVariantSelectionSets(const Path& primPath) const {
  std::vector<std::string> out;
  if (!primPath.IsPrimPath()) {
    return out;
  }
  const auto it = prim_variant_selection_.find(primPath);
  if (it == prim_variant_selection_.end()) {
    return out;
  }
  out.reserve(it->second.size());
  for (const auto& e : it->second) {
    out.push_back(e.first);
  }
  std::sort(out.begin(), out.end());
  return out;
}

void Layer::ClearPrimVariantSets(const Path& primPath) {
  prim_variant_sets_.erase(primPath);
}

void Layer::SetPrimVariantSetVariants(const Path& primPath, std::string variantSetName,
                                      std::vector<std::string> variantNames) {
  if (!primPath.IsPrimPath() || variantSetName.empty()) {
    return;
  }
  touch_hierarchy(primPath);
  auto& by_set = prim_variant_sets_[primPath];
  if (variantNames.empty()) {
    by_set.erase(variantSetName);
    if (by_set.empty()) {
      prim_variant_sets_.erase(primPath);
    }
    return;
  }
  by_set[std::move(variantSetName)] = std::move(variantNames);
}

bool Layer::HasPrimVariantSet(const Path& primPath, const std::string& variantSetName) const {
  if (!primPath.IsPrimPath() || variantSetName.empty()) {
    return false;
  }
  const auto it = prim_variant_sets_.find(primPath);
  if (it == prim_variant_sets_.end()) {
    return false;
  }
  return it->second.find(variantSetName) != it->second.end();
}

std::vector<std::string> Layer::ListPrimVariantSetNames(const Path& primPath) const {
  std::vector<std::string> out;
  if (!primPath.IsPrimPath()) {
    return out;
  }
  const auto it = prim_variant_sets_.find(primPath);
  if (it == prim_variant_sets_.end()) {
    return out;
  }
  out.reserve(it->second.size());
  for (const auto& e : it->second) {
    out.push_back(e.first);
  }
  std::sort(out.begin(), out.end());
  return out;
}

std::vector<std::string> Layer::ListPrimVariantNames(const Path& primPath, const std::string& variantSetName) const {
  if (!primPath.IsPrimPath() || variantSetName.empty()) {
    return {};
  }
  const auto it = prim_variant_sets_.find(primPath);
  if (it == prim_variant_sets_.end()) {
    return {};
  }
  const auto jt = it->second.find(variantSetName);
  if (jt == it->second.end()) {
    return {};
  }
  return jt->second;
}

}  // namespace freeusd::sdf
