#pragma once

#include "freeusd/tf/token.hpp"

namespace freeusd::usdPhysics::tokens {

inline freeusd::tf::Token Scene() { return freeusd::tf::Token("PhysicsScene"); }
inline freeusd::tf::Token RigidBodyAPI() { return freeusd::tf::Token("RigidBodyAPI"); }
inline freeusd::tf::Token CollisionAPI() { return freeusd::tf::Token("CollisionAPI"); }

}  // namespace freeusd::usdPhysics::tokens
