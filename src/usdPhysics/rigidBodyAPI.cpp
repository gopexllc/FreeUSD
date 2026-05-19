#include "freeusd/usdPhysics/rigidBodyAPI.hpp"

#include "freeusd/usd/stage.hpp"
#include "freeusd/usdPhysics/tokens.hpp"

namespace freeusd::usdPhysics {
namespace {

bool prim_has_composed_api_schema(const freeusd::usd::Prim& prim, const freeusd::tf::Token& schema) {
  const std::shared_ptr<const freeusd::usd::Stage> stage = prim.GetStage();
  if (!stage || schema.IsEmpty()) {
    return false;
  }
  freeusd::vt::Value v;
  if (!stage->ReadFieldAtEvaluatedTime(prim.GetPath(), freeusd::tf::Token{"apiSchemas"}, 1.0, &v)) {
    return false;
  }
  std::vector<freeusd::tf::Token> schemas;
  if (!v.GetTokenArray(&schemas)) {
    return false;
  }
  for (const freeusd::tf::Token& entry : schemas) {
    if (entry == schema) {
      return true;
    }
  }
  return false;
}

}  // namespace

RigidBodyAPI RigidBodyAPI::ReadFromPrim(const std::shared_ptr<const freeusd::usd::Stage>& stage,
                                        const freeusd::sdf::Path& path) {
  if (!stage) {
    return {};
  }
  return RigidBodyAPI(stage->GetPrimAtPath(path));
}

bool RigidBodyAPI::IsRigidBodyAPI() const {
  return prim.IsValid() &&
         (prim.HasAttribute(tokens::physics_mass()) ||
          prim_has_composed_api_schema(prim, tokens::PhysicsRigidBodyAPI()));
}

bool RigidBodyAPI::GetMass(float* out, double time) const {
  if (!out || !IsRigidBodyAPI()) {
    return false;
  }
  return prim.GetAttribute(tokens::physics_mass(), time).GetFloat(out);
}

}  // namespace freeusd::usdPhysics
