#include "freeusd/usd/stage.hpp"

#include <algorithm>

namespace freeusd::usd {

Stage::Stage(std::shared_ptr<freeusd::sdf::Layer> root) : root_(std::move(root)) {
  if (!resolver_) {
    resolver_ = std::make_unique<freeusd::ar::DefaultResolver>();
  }
}

std::shared_ptr<Stage> Stage::AttachRootLayer(std::shared_ptr<freeusd::sdf::Layer> root) {
  if (!root) {
    return {};
  }
  return std::shared_ptr<Stage>(new Stage(std::move(root)));
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
  const auto paths = root_->ListPrimPaths();
  for (const auto& p : paths) {
    if (!p.IsPrimPath()) {
      continue;
    }
    if (p.GetParentPath() == primPath) {
      out.emplace_back(weak_from_this(), p);
    }
  }
  std::sort(out.begin(), out.end(), [](const Prim& a, const Prim& b) { return a.GetPath().GetString() < b.GetPath().GetString(); });
  return out;
}

void Stage::SetResolver(std::unique_ptr<freeusd::ar::Resolver> resolver) {
  if (resolver) {
    resolver_ = std::move(resolver);
  }
}

}  // namespace freeusd::usd
