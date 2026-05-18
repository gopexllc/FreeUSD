#include "freeusd/usdPhysics/physicsScene.hpp"

#include "freeusd/usd/stage.hpp"
#include "freeusd/usdPhysics/tokens.hpp"

namespace freeusd::usdPhysics {

PhysicsScene PhysicsScene::ReadFromPrim(const std::shared_ptr<const freeusd::usd::Stage>& stage,
                                        const freeusd::sdf::Path& path) {
  if (!stage) {
    return {};
  }
  return PhysicsScene(stage->GetPrimAtPath(path));
}

bool PhysicsScene::IsPhysicsScene() const {
  return prim.IsValid() && prim.HasPrimKind() && prim.GetPrimKind() == tokens::PhysicsScene();
}

bool PhysicsScene::GetGravityDirection(freeusd::gf::Vec3f* out, double time) const {
  if (!out || !IsPhysicsScene()) {
    return false;
  }
  return prim.GetAttribute(tokens::physics_gravityDirection(), time).GetVec3f(out);
}

bool PhysicsScene::GetGravityMagnitude(float* out, double time) const {
  if (!out || !IsPhysicsScene()) {
    return false;
  }
  return prim.GetAttribute(tokens::physics_gravityMagnitude(), time).GetFloat(out);
}

}  // namespace freeusd::usdPhysics
