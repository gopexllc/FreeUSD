#include "freeusd/usd/prim.hpp"

#include <algorithm>

#include "freeusd/usd/stage.hpp"

namespace freeusd::usd {

Prim::Prim(std::weak_ptr<const Stage> stage, freeusd::sdf::Path path) : stage_(std::move(stage)), path_(std::move(path)) {}

std::shared_ptr<const Stage> Prim::lock_stage() const { return stage_.lock(); }

bool Prim::IsValid() const noexcept {
  const auto st = lock_stage();
  if (!st || !path_.IsPrimPath()) {
    return false;
  }
  const auto paths = st->GetRootLayer().ListPrimPaths();
  return std::find(paths.begin(), paths.end(), path_) != paths.end();
}

bool Prim::HasAttribute(const freeusd::tf::Token& name) const {
  const auto st = lock_stage();
  if (!st || name.IsEmpty()) {
    return false;
  }
  return st->GetRootLayer().HasField(path_, name);
}

freeusd::vt::Value Prim::GetAttribute(const freeusd::tf::Token& name) const {
  const auto st = lock_stage();
  freeusd::vt::Value v;
  if (!st || name.IsEmpty()) {
    return v;
  }
  // Default stage time 1.0 (UsdTimeCode-like); hold + default resolution.
  if (!st->GetRootLayer().GetFieldAtTime(path_, name, 1.0, &v)) {
    st->GetRootLayer().GetField(path_, name, &v);
  }
  return v;
}

}  // namespace freeusd::usd
