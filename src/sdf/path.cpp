#include "freeusd/sdf/path.hpp"

#include <algorithm>
#include <utility>

namespace freeusd::sdf {
namespace {

bool is_valid_prim_segment(std::string_view seg) {
  if (seg.empty()) {
    return false;
  }
  for (char c : seg) {
    const bool ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_';
    if (!ok) {
      return false;
    }
  }
  return true;
}

bool is_valid_property_tail(std::string_view tail) {
  if (tail.empty()) {
    return false;
  }
  for (char c : tail) {
    const bool ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_' ||
                    c == ':' || c == '.';
    if (!ok) {
      return false;
    }
  }
  return true;
}

/// Validates one prim path component, optionally with a single `{variantSet=variantName}` suffix.
bool parse_prim_segment(std::string_view seg, std::string_view* base, std::string_view* variant_set,
                        std::string_view* variant_name) {
  const std::size_t open = seg.find('{');
  if (open == std::string_view::npos) {
    if (!is_valid_prim_segment(seg)) {
      return false;
    }
    *base = seg;
    *variant_set = {};
    *variant_name = {};
    return true;
  }
  if (seg.size() < 3 || seg.back() != '}') {
    return false;
  }
  if (seg.find('{', open + 1) != std::string_view::npos) {
    return false;
  }
  const std::string_view inner = seg.substr(open + 1, seg.size() - open - 2);
  const std::size_t eq = inner.find('=');
  if (eq == std::string_view::npos || eq == 0 || eq + 1 >= inner.size()) {
    return false;
  }
  if (inner.find('=', eq + 1) != std::string_view::npos) {
    return false;
  }
  const std::string_view b = seg.substr(0, open);
  const std::string_view vs = inner.substr(0, eq);
  const std::string_view vn = inner.substr(eq + 1);
  if (!is_valid_prim_segment(b) || !is_valid_prim_segment(vs) || !is_valid_prim_segment(vn)) {
    return false;
  }
  *base = b;
  *variant_set = vs;
  *variant_name = vn;
  return true;
}

bool validate_prim_path_components(std::string_view norm_prim) {
  if (norm_prim.empty() || norm_prim[0] != '/') {
    return false;
  }
  if (norm_prim == "/") {
    return true;
  }
  std::string_view rest = norm_prim;
  rest.remove_prefix(1);
  while (!rest.empty()) {
    const std::size_t p = rest.find('/');
    const std::string_view seg = rest.substr(0, p == std::string_view::npos ? rest.size() : p);
    std::string_view base;
    std::string_view vs;
    std::string_view vn;
    if (!parse_prim_segment(seg, &base, &vs, &vn)) {
      return false;
    }
    if (p == std::string_view::npos) {
      break;
    }
    rest.remove_prefix(p + 1);
  }
  return true;
}

std::string normalize(std::string_view in) {
  if (in.empty()) {
    return {};
  }
  std::string s{in};
  // Collapse duplicate slashes.
  std::string out;
  out.reserve(s.size());
  bool prev_slash = false;
  for (char c : s) {
    if (c == '/') {
      if (!prev_slash || out.empty()) {
        out.push_back(c);
      }
      prev_slash = true;
    } else {
      prev_slash = false;
      out.push_back(c);
    }
  }
  if (out.size() > 1 && out.back() == '/') {
    out.pop_back();
  }
  return out;
}

bool validate_normalized_absolute(const std::string& norm) {
  if (norm.empty()) {
    return false;
  }
  if (norm[0] != '/') {
    return false;
  }
  if (norm.find("..") != std::string::npos) {
    return false;
  }
  if (norm.find('[') != std::string::npos || norm.find(']') != std::string::npos) {
    return false;
  }

  // Split prim/property: last `/` segment may contain first `.` splitting prim name and property name.
  const std::size_t last_slash = norm.rfind('/');
  const std::string_view tail = std::string_view{norm}.substr(last_slash == std::string::npos ? 0 : last_slash + 1);
  if (tail.empty() && norm != "/") {
    return false;
  }
  if (norm == "/") {
    return true;
  }

  const std::size_t dot = std::string_view{tail}.find('.');
  if (dot == std::string_view::npos) {
    return validate_prim_path_components(norm);
  }

  // Property path: split tail at first dot -> primName, propertyRest
  const std::string_view prim_name = std::string_view{tail}.substr(0, dot);
  const std::string_view prop = std::string_view{tail}.substr(dot + 1);
  std::string_view base;
  std::string_view vs;
  std::string_view vn;
  if (!parse_prim_segment(prim_name, &base, &vs, &vn) || prop.empty() || !is_valid_property_tail(prop)) {
    return false;
  }
  const std::string_view prefix = std::string_view{norm}.substr(0, last_slash + 1 + prim_name.size());
  // Validate prefix as prim path.
  return validate_normalized_absolute(std::string{prefix});
}

}  // namespace

Path Path::AbsoluteRootPath() { return Path{std::string{"/"}}; }

Path Path::FromString(std::string_view text) {
  const std::string norm = normalize(text);
  if (norm.empty() || !validate_normalized_absolute(norm)) {
    return Path{};
  }
  return Path{norm};
}

bool Path::IsPrimPath() const noexcept {
  if (IsEmpty() || !IsAbsolutePath()) {
    return false;
  }
  const std::size_t last_slash = text_.rfind('/');
  const std::string_view tail =
      last_slash == std::string::npos ? std::string_view{text_} : std::string_view{text_}.substr(last_slash + 1);
  return !tail.empty() && tail.find('.') == std::string_view::npos;
}

bool Path::IsPropertyPath() const noexcept {
  if (IsEmpty() || !IsAbsolutePath() || IsAbsoluteRootPath()) {
    return false;
  }
  const std::size_t last_slash = text_.rfind('/');
  const std::string_view tail =
      last_slash == std::string::npos ? std::string_view{text_} : std::string_view{text_}.substr(last_slash + 1);
  return tail.find('.') != std::string_view::npos;
}

std::size_t Path::GetPathElementCount() const {
  if (IsEmpty() || !IsAbsolutePath()) {
    return 0;
  }
  const Path prim = GetPrimPath();
  if (prim.IsAbsoluteRootPath()) {
    return 0;
  }
  std::size_t n = 0;
  for (std::size_t i = 1; i < prim.text_.size(); ++i) {
    if (prim.text_[i] == '/') {
      ++n;
    }
  }
  return n + 1;
}

Path Path::GetPrimPath() const {
  if (!IsPropertyPath()) {
    return *this;
  }
  const std::size_t last_slash = text_.rfind('/');
  const std::string_view full = text_;
  const std::string_view tail = full.substr(last_slash + 1);
  const std::size_t dot = tail.find('.');
  const std::string_view prim_name = tail.substr(0, dot);
  const std::string prefix = std::string{full.substr(0, last_slash + 1)} + std::string{prim_name};
  return Path{prefix};
}

Path Path::GetParentPath() const {
  if (IsEmpty()) {
    return {};
  }
  if (IsPropertyPath()) {
    return GetPrimPath();
  }
  if (IsAbsoluteRootPath()) {
    return {};
  }
  const std::size_t last_slash = text_.rfind('/');
  if (last_slash == 0) {
    return AbsoluteRootPath();
  }
  if (last_slash == std::string::npos) {
    return {};
  }
  return Path{text_.substr(0, last_slash)};
}

std::string Path::GetName() const {
  if (IsEmpty()) {
    return {};
  }
  const std::size_t last_slash = text_.rfind('/');
  const std::string_view tail =
      last_slash == std::string::npos ? std::string_view{text_} : std::string_view{text_}.substr(last_slash + 1);
  if (const std::size_t dot = tail.find('.'); dot != std::string_view::npos) {
    return std::string{tail.substr(dot + 1)};
  }
  std::string_view base;
  std::string_view vs;
  std::string_view vn;
  if (parse_prim_segment(tail, &base, &vs, &vn)) {
    return std::string{base};
  }
  return std::string{tail};
}

std::vector<PrimVariantSelection> Path::GetVariantSelections() const {
  std::vector<PrimVariantSelection> out;
  const Path prim = GetPrimPath();
  if (prim.IsEmpty() || !prim.IsAbsolutePath() || prim.IsAbsoluteRootPath()) {
    return out;
  }
  std::string_view rest = prim.text_;
  rest.remove_prefix(1);
  while (!rest.empty()) {
    const std::size_t p = rest.find('/');
    const std::string_view seg = rest.substr(0, p == std::string_view::npos ? rest.size() : p);
    std::string_view base;
    std::string_view vs;
    std::string_view vn;
    if (!parse_prim_segment(seg, &base, &vs, &vn)) {
      return {};
    }
    if (!vs.empty() && !vn.empty()) {
      out.push_back(PrimVariantSelection{std::string{vs}, std::string{vn}});
    }
    if (p == std::string_view::npos) {
      break;
    }
    rest.remove_prefix(p + 1);
  }
  return out;
}

bool Path::HasPrefix(const Path& prefix) const {
  if (IsEmpty() || prefix.IsEmpty()) {
    return false;
  }
  if (prefix.text_.size() > text_.size()) {
    return false;
  }
  if (text_.compare(0, prefix.text_.size(), prefix.text_) != 0) {
    return false;
  }
  if (prefix.text_.size() == text_.size()) {
    return true;
  }
  const char next = text_[prefix.text_.size()];
  return next == '/';
}

Path Path::AppendChild(const freeusd::tf::Token& name) const {
  return AppendChild(name.GetText());
}

Path Path::AppendChild(std::string_view name) const {
  if (!is_valid_prim_segment(name)) {
    return {};
  }
  if (!IsAbsolutePath() || IsPropertyPath()) {
    return {};
  }
  if (IsAbsoluteRootPath()) {
    return Path{std::string("/") + std::string{name}};
  }
  return Path{text_ + "/" + std::string{name}};
}

std::vector<Path> PathGetPrefixes(const Path& path) {
  std::vector<Path> out;
  if (path.IsEmpty()) {
    return out;
  }
  out.push_back(Path::AbsoluteRootPath());
  const Path prim = path.GetPrimPath();
  if (prim.IsAbsoluteRootPath()) {
    return out;
  }
  const std::string& s = prim.GetString();
  for (std::size_t i = 1; i < s.size(); ++i) {
    if (s[i] == '/') {
      out.push_back(Path::FromString(s.substr(0, i)));
    }
  }
  out.push_back(prim);
  if (path.IsPropertyPath() && path.GetString() != prim.GetString()) {
    out.push_back(path);
  }
  return out;
}

}  // namespace freeusd::sdf
