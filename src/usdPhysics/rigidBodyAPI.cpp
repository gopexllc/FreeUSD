#include "freeusd/usdPhysics/rigidBodyAPI.hpp"

#include "freeusd/usd/stage.hpp"
#include "freeusd/usdPhysics/tokens.hpp"

namespace freeusd::usdPhysics {

RigidBodyAPI RigidBodyAPI::ReadFromPrim(const std::shared_ptr<const freeusd::usd::Stage>& stage,
                                        const freeusd::sdf::Path& path) {
  if (!stage) {
    return {};
  }
  return RigidBodyAPI(stage->GetPrimAtPath(path));
}

bool RigidBodyAPI::IsRigidBodyAPI() const {
  return prim.IsValid() && prim.HasAttribute(tokens::physics_mass());
}

bool RigidBodyAPI::GetMass(float* out, double time) const {
  if (!out || !IsRigidBodyAPI()) {
    return false;
  }
  return prim.GetAttribute(tokens::physics_mass(), time).GetFloat(out);
}

}  // namespace freeusd::usdPhysics
