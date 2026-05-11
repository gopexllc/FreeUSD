#include "freeusd/usdGeom/imageable.hpp"

#include "freeusd/tf/token.hpp"
#include "freeusd/vt/value.hpp"

namespace freeusd::usdGeom {
namespace {

bool read_token_like(const freeusd::vt::Value& value, std::string* out) {
  if (!out) {
    return false;
  }
  freeusd::tf::Token token;
  if (value.GetToken(&token) && !token.IsEmpty()) {
    *out = token.GetText();
    return true;
  }
  std::string text;
  if (value.GetString(&text) && !text.empty()) {
    *out = std::move(text);
    return true;
  }
  return false;
}

}  // namespace

bool Imageable::ComputeVisibility(double time) const {
  if (!prim.IsValid()) {
    return true;
  }
  for (freeusd::usd::Prim cur = prim; cur.IsValid(); cur = cur.GetParent()) {
    std::string vis;
    if (read_token_like(cur.GetAttribute(freeusd::tf::Token("visibility"), time), &vis) && vis == "invisible") {
      return false;
    }
  }
  return true;
}

std::string Imageable::ComputePurpose(double time) const {
  if (!prim.IsValid()) {
    return "default";
  }
  for (freeusd::usd::Prim cur = prim; cur.IsValid(); cur = cur.GetParent()) {
    std::string purpose;
    if (!read_token_like(cur.GetAttribute(freeusd::tf::Token("purpose"), time), &purpose)) {
      continue;
    }
    if (purpose.empty() || purpose == "inherited") {
      continue;
    }
    return purpose;
  }
  return "default";
}

}  // namespace freeusd::usdGeom
