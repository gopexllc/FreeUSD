#include "freeusd/usdSkel/skelRoot.hpp"

#include <optional>
#include <vector>

#include "freeusd/usd/stage.hpp"
#include "freeusd/usdSkel/tokens.hpp"

namespace freeusd::usdSkel {
namespace {

std::optional<freeusd::sdf::Path> path_from_relationship_or_token(const freeusd::usd::Prim& prim,
                                                                   const freeusd::tf::Token& name) {
  if (!prim.IsValid()) {
    return std::nullopt;
  }
  const std::vector<freeusd::sdf::Path> targets = prim.GetRelationshipTargets(name);
  if (!targets.empty() && !targets[0].IsEmpty()) {
    return targets[0];
  }
  const freeusd::vt::Value v = prim.GetAttribute(name, 1.0);
  freeusd::tf::Token tok;
  if (v.GetToken(&tok) && !tok.IsEmpty()) {
    const freeusd::sdf::Path p = freeusd::sdf::Path::FromString(tok.GetText());
    if (!p.IsEmpty()) {
      return p;
    }
  }
  return std::nullopt;
}

std::optional<freeusd::sdf::Path> find_skeleton_under(const freeusd::usd::Prim& root) {
  if (!root.IsValid()) {
    return std::nullopt;
  }
  if (root.GetPrimKind() == tokens::Skeleton()) {
    return root.GetPath();
  }
  for (const freeusd::usd::Prim& child : root.GetChildren()) {
    if (const std::optional<freeusd::sdf::Path> found = find_skeleton_under(child)) {
      return found;
    }
  }
  return std::nullopt;
}

}  // namespace

SkelRoot SkelRoot::ReadFromPrim(const std::shared_ptr<const freeusd::usd::Stage>& stage,
                                const freeusd::sdf::Path& path) {
  if (!stage) {
    return {};
  }
  return SkelRoot(stage->GetPrimAtPath(path));
}

std::optional<freeusd::sdf::Path> SkelRoot::FindSkeletonPath() const {
  return find_skeleton_under(prim);
}

std::optional<freeusd::sdf::Path> SkelRoot::GetAnimationSourcePath() const {
  return path_from_relationship_or_token(prim, tokens::skel_animationSource());
}

Skeleton SkelRoot::GetSkeleton() const {
  if (const std::optional<freeusd::sdf::Path> path = FindSkeletonPath()) {
    return Skeleton(prim.GetStage()->GetPrimAtPath(*path));
  }
  return {};
}

SkelAnimation SkelRoot::GetAnimationSource() const {
  if (const std::optional<freeusd::sdf::Path> path = GetAnimationSourcePath()) {
    return SkelAnimation(prim.GetStage()->GetPrimAtPath(*path));
  }
  return {};
}

}  // namespace freeusd::usdSkel
