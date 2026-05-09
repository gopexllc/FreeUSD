#include "freeusd/sdf/primReference.hpp"

#include <cctype>

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

bool PrimReference::ParseAuthored(std::string_view text, PrimReference* out) {
  if (!out) {
    return false;
  }
  *out = PrimReference{};
  text = trim_sv(text);
  if (text.empty()) {
    return false;
  }
  if (text.size() >= 2 && text.front() == '"' && text.back() == '"') {
    text = trim_sv(text.substr(1, text.size() - 2));
    if (text.empty()) {
      return false;
    }
    out->asset_path = std::string{text};
    return true;
  }
  if (!text.empty() && text.front() == '@') {
    if (text.size() < 3) {
      return false;
    }
    const std::size_t end_at = text.find('@', 1);
    if (end_at == std::string_view::npos) {
      return false;
    }
    std::string_view asset = trim_sv(text.substr(1, end_at - 1));
    if (asset.empty()) {
      return false;
    }
    out->asset_path = std::string{asset};
    std::string_view tail = trim_sv(text.substr(end_at + 1));
    if (!tail.empty()) {
      if (tail.size() < 3 || tail.front() != '<' || tail.back() != '>') {
        return false;
      }
      const std::string_view inner = trim_sv(tail.substr(1, tail.size() - 2));
      const Path p = Path::FromString(inner);
      if (p.IsEmpty()) {
        return false;
      }
      out->prim_path = p;
    }
    return true;
  }
  out->asset_path = std::string{text};
  return true;
}

std::string PrimReference::FormatAuthoredForUsda() const {
  std::string s;
  s.reserve(asset_path.size() + 8 + (prim_path.has_value() ? prim_path->GetText().size() + 2 : 0));
  s.push_back('@');
  s += asset_path;
  s.push_back('@');
  if (prim_path.has_value() && !prim_path->IsEmpty()) {
    s.push_back('<');
    s += prim_path->GetText();
    s.push_back('>');
  }
  return s;
}

}  // namespace freeusd::sdf
