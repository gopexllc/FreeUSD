#include "freeusd/usdPhysics/fixedJoint.hpp"

#include "freeusd/usd/stage.hpp"
#include "freeusd/usdPhysics/tokens.hpp"

namespace freeusd::usdPhysics {

FixedJoint FixedJoint::ReadFromPrim(const std::shared_ptr<const freeusd::usd::Stage>& stage,
                                    const freeusd::sdf::Path& path) {
  if (!stage) {
    return {};
  }
  return FixedJoint(stage->GetPrimAtPath(path));
}

bool FixedJoint::IsFixedJoint() const {
  return prim.IsValid() && prim.HasPrimKind() && prim.GetPrimKind() == tokens::PhysicsFixedJoint();
}

bool FixedJoint::GetBody0(freeusd::sdf::Path* out) const {
  if (!out || !IsFixedJoint()) {
    return false;
  }
  const std::vector<freeusd::sdf::Path> targets = prim.GetRelationshipTargets(tokens::physics_body0());
  if (targets.empty()) {
    return false;
  }
  *out = targets.front();
  return true;
}

bool FixedJoint::GetBody1(freeusd::sdf::Path* out) const {
  if (!out || !IsFixedJoint()) {
    return false;
  }
  const std::vector<freeusd::sdf::Path> targets = prim.GetRelationshipTargets(tokens::physics_body1());
  if (targets.empty()) {
    return false;
  }
  *out = targets.front();
  return true;
}

bool FixedJoint::GetJointEnabled(bool* out, double time) const {
  if (!out || !IsFixedJoint()) {
    return false;
  }
  return prim.GetAttribute(tokens::physics_jointEnabled(), time).GetBool(out);
}

}  // namespace freeusd::usdPhysics
