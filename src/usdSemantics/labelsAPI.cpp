#include "freeusd/usdSemantics/labelsAPI.hpp"

#include <algorithm>

#include "freeusd/tf/token.hpp"
#include "freeusd/usd/stage.hpp"

namespace freeusd::usdSemantics {
namespace {

static constexpr char kLabelAttrPrefix[] = "semantics:labels:";

freeusd::tf::Token labels_attr_token(const std::string& instance_name) {
  if (instance_name.empty()) {
    return {};
  }
  return freeusd::tf::Token(std::string(kLabelAttrPrefix) + instance_name);
}

bool starts_with_label_prefix(const std::string& name) {
  constexpr std::size_t prefix_len = sizeof(kLabelAttrPrefix) - 1;
  return name.size() > prefix_len && name.compare(0, prefix_len, kLabelAttrPrefix) == 0;
}

}  // namespace

SemanticLabelsAPI SemanticLabelsAPI::ReadFromPrim(const std::shared_ptr<const freeusd::usd::Stage>& stage,
                                                  const freeusd::sdf::Path& path) {
  if (!stage) {
    return {};
  }
  return SemanticLabelsAPI(stage->GetPrimAtPath(path));
}

bool SemanticLabelsAPI::HasLabels(const std::string& instance_name) const {
  if (!prim.IsValid()) {
    return false;
  }
  const freeusd::tf::Token attr = labels_attr_token(instance_name);
  return !attr.IsEmpty() && prim.HasAttribute(attr);
}

std::vector<std::string> SemanticLabelsAPI::ListLabelSetNames() const {
  std::vector<std::string> out;
  if (!prim.IsValid()) {
    return out;
  }
  constexpr std::size_t prefix_len = sizeof(kLabelAttrPrefix) - 1;
  for (const std::string& name : prim.ListAttributeNames()) {
    if (starts_with_label_prefix(name)) {
      out.push_back(name.substr(prefix_len));
    }
  }
  std::sort(out.begin(), out.end());
  out.erase(std::unique(out.begin(), out.end()), out.end());
  return out;
}

std::vector<std::string> SemanticLabelsAPI::GetLabels(const std::string& instance_name, double time) const {
  std::vector<std::string> out;
  if (!prim.IsValid()) {
    return out;
  }
  const freeusd::tf::Token attr = labels_attr_token(instance_name);
  if (attr.IsEmpty()) {
    return out;
  }
  std::vector<freeusd::tf::Token> labels;
  if (!prim.GetAttribute(attr, time).GetTokenArray(&labels)) {
    return out;
  }
  out.reserve(labels.size());
  for (const freeusd::tf::Token& label : labels) {
    if (!label.IsEmpty()) {
      out.push_back(label.GetText());
    }
  }
  return out;
}

}  // namespace freeusd::usdSemantics
