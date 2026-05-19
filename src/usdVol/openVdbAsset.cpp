#include "freeusd/usdVol/openVdbAsset.hpp"

#include <optional>

#include "freeusd/usd/stage.hpp"
#include "freeusd/usdVol/tokens.hpp"

namespace freeusd::usdVol {
namespace {

std::optional<std::string> normalize_usd_asset_path_string(const std::string& s) {
  if (s.empty()) {
    return std::nullopt;
  }
  if (s.size() >= 2u && s.front() == '@' && s.back() == '@') {
    return s.substr(1, s.size() - 2u);
  }
  return s;
}

bool read_asset_path_from_value(const freeusd::vt::Value& v, std::string* out_path) {
  if (!out_path) {
    return false;
  }
  std::string text;
  if (!v.GetString(&text)) {
    return false;
  }
  const std::optional<std::string> normalized = normalize_usd_asset_path_string(text);
  if (!normalized || normalized->empty()) {
    return false;
  }
  *out_path = *normalized;
  return true;
}

bool read_token_string_from_value(const freeusd::vt::Value& v, std::string* out_text) {
  if (!out_text) {
    return false;
  }
  return v.GetString(out_text) && !out_text->empty();
}

}  // namespace

OpenVDBAsset OpenVDBAsset::ReadFromPrim(const std::shared_ptr<const freeusd::usd::Stage>& stage,
                                        const freeusd::sdf::Path& path) {
  if (!stage) {
    return {};
  }
  return OpenVDBAsset(stage->GetPrimAtPath(path));
}

bool OpenVDBAsset::IsOpenVDBAsset() const {
  return prim.IsValid() && prim.HasPrimKind() && prim.GetPrimKind() == tokens::OpenVDBAsset();
}

bool OpenVDBAsset::GetFilePath(std::string* out_path, double time) const {
  if (!out_path || !IsOpenVDBAsset()) {
    return false;
  }
  out_path->clear();
  return read_asset_path_from_value(prim.GetAttribute(tokens::filePath(), time), out_path);
}

bool OpenVDBAsset::GetFieldName(std::string* out_name, double time) const {
  if (!out_name || !IsOpenVDBAsset()) {
    return false;
  }
  out_name->clear();
  return read_token_string_from_value(prim.GetAttribute(tokens::fieldName(), time), out_name);
}

}  // namespace freeusd::usdVol
