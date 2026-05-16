#pragma once

// GENERATED FILE - do not edit by hand. Regenerate with: python3 scripts/gen_schema_tokens.py
// Token strings derived from OpenUSD published schema data (https://raw.githubusercontent.com/PixarAnimationStudios/OpenUSD/dev/pxr/usd/.../generatedSchema.usda).

#include "freeusd/tf/token.hpp"

namespace freeusd::usdPhysics::tokens {

/// Schema tokens (OpenUSD-shaped names; clean-room).
inline freeusd::tf::Token PhysicsArticulationRootAPI() { return freeusd::tf::Token("PhysicsArticulationRootAPI"); }
inline freeusd::tf::Token PhysicsCollisionAPI() { return freeusd::tf::Token("PhysicsCollisionAPI"); }
inline freeusd::tf::Token PhysicsCollisionGroup() { return freeusd::tf::Token("PhysicsCollisionGroup"); }
inline freeusd::tf::Token PhysicsDistanceJoint() { return freeusd::tf::Token("PhysicsDistanceJoint"); }
inline freeusd::tf::Token PhysicsDriveAPI() { return freeusd::tf::Token("PhysicsDriveAPI"); }
inline freeusd::tf::Token PhysicsFilteredPairsAPI() { return freeusd::tf::Token("PhysicsFilteredPairsAPI"); }
inline freeusd::tf::Token PhysicsFixedJoint() { return freeusd::tf::Token("PhysicsFixedJoint"); }
inline freeusd::tf::Token PhysicsJoint() { return freeusd::tf::Token("PhysicsJoint"); }
inline freeusd::tf::Token PhysicsLimitAPI() { return freeusd::tf::Token("PhysicsLimitAPI"); }
inline freeusd::tf::Token PhysicsMassAPI() { return freeusd::tf::Token("PhysicsMassAPI"); }
inline freeusd::tf::Token PhysicsMaterialAPI() { return freeusd::tf::Token("PhysicsMaterialAPI"); }
inline freeusd::tf::Token PhysicsMeshCollisionAPI() { return freeusd::tf::Token("PhysicsMeshCollisionAPI"); }
inline freeusd::tf::Token PhysicsPrismaticJoint() { return freeusd::tf::Token("PhysicsPrismaticJoint"); }
inline freeusd::tf::Token PhysicsRevoluteJoint() { return freeusd::tf::Token("PhysicsRevoluteJoint"); }
inline freeusd::tf::Token PhysicsRigidBodyAPI() { return freeusd::tf::Token("PhysicsRigidBodyAPI"); }
inline freeusd::tf::Token PhysicsScene() { return freeusd::tf::Token("PhysicsScene"); }
inline freeusd::tf::Token PhysicsSphericalJoint() { return freeusd::tf::Token("PhysicsSphericalJoint"); }
inline freeusd::tf::Token drive___INSTANCE_NAME___physics_damping() { return freeusd::tf::Token("drive:__INSTANCE_NAME__:physics:damping"); }
inline freeusd::tf::Token drive___INSTANCE_NAME___physics_maxForce() { return freeusd::tf::Token("drive:__INSTANCE_NAME__:physics:maxForce"); }
inline freeusd::tf::Token drive___INSTANCE_NAME___physics_stiffness() { return freeusd::tf::Token("drive:__INSTANCE_NAME__:physics:stiffness"); }
inline freeusd::tf::Token drive___INSTANCE_NAME___physics_targetPosition() { return freeusd::tf::Token("drive:__INSTANCE_NAME__:physics:targetPosition"); }
inline freeusd::tf::Token drive___INSTANCE_NAME___physics_targetVelocity() { return freeusd::tf::Token("drive:__INSTANCE_NAME__:physics:targetVelocity"); }
inline freeusd::tf::Token drive___INSTANCE_NAME___physics_type() { return freeusd::tf::Token("drive:__INSTANCE_NAME__:physics:type"); }
inline freeusd::tf::Token limit___INSTANCE_NAME___physics_high() { return freeusd::tf::Token("limit:__INSTANCE_NAME__:physics:high"); }
inline freeusd::tf::Token limit___INSTANCE_NAME___physics_low() { return freeusd::tf::Token("limit:__INSTANCE_NAME__:physics:low"); }
inline freeusd::tf::Token physics_angularVelocity() { return freeusd::tf::Token("physics:angularVelocity"); }
inline freeusd::tf::Token physics_approximation() { return freeusd::tf::Token("physics:approximation"); }
inline freeusd::tf::Token physics_axis() { return freeusd::tf::Token("physics:axis"); }
inline freeusd::tf::Token physics_body0() { return freeusd::tf::Token("physics:body0"); }
inline freeusd::tf::Token physics_body1() { return freeusd::tf::Token("physics:body1"); }
inline freeusd::tf::Token physics_breakForce() { return freeusd::tf::Token("physics:breakForce"); }
inline freeusd::tf::Token physics_breakTorque() { return freeusd::tf::Token("physics:breakTorque"); }
inline freeusd::tf::Token physics_centerOfMass() { return freeusd::tf::Token("physics:centerOfMass"); }
inline freeusd::tf::Token physics_collisionEnabled() { return freeusd::tf::Token("physics:collisionEnabled"); }
inline freeusd::tf::Token physics_coneAngle0Limit() { return freeusd::tf::Token("physics:coneAngle0Limit"); }
inline freeusd::tf::Token physics_coneAngle1Limit() { return freeusd::tf::Token("physics:coneAngle1Limit"); }
inline freeusd::tf::Token physics_density() { return freeusd::tf::Token("physics:density"); }
inline freeusd::tf::Token physics_diagonalInertia() { return freeusd::tf::Token("physics:diagonalInertia"); }
inline freeusd::tf::Token physics_dynamicFriction() { return freeusd::tf::Token("physics:dynamicFriction"); }
inline freeusd::tf::Token physics_excludeFromArticulation() { return freeusd::tf::Token("physics:excludeFromArticulation"); }
inline freeusd::tf::Token physics_filteredGroups() { return freeusd::tf::Token("physics:filteredGroups"); }
inline freeusd::tf::Token physics_filteredPairs() { return freeusd::tf::Token("physics:filteredPairs"); }
inline freeusd::tf::Token physics_gravityDirection() { return freeusd::tf::Token("physics:gravityDirection"); }
inline freeusd::tf::Token physics_gravityMagnitude() { return freeusd::tf::Token("physics:gravityMagnitude"); }
inline freeusd::tf::Token physics_invertFilteredGroups() { return freeusd::tf::Token("physics:invertFilteredGroups"); }
inline freeusd::tf::Token physics_jointEnabled() { return freeusd::tf::Token("physics:jointEnabled"); }
inline freeusd::tf::Token physics_kinematicEnabled() { return freeusd::tf::Token("physics:kinematicEnabled"); }
inline freeusd::tf::Token physics_localPos0() { return freeusd::tf::Token("physics:localPos0"); }
inline freeusd::tf::Token physics_localPos1() { return freeusd::tf::Token("physics:localPos1"); }
inline freeusd::tf::Token physics_localRot0() { return freeusd::tf::Token("physics:localRot0"); }
inline freeusd::tf::Token physics_localRot1() { return freeusd::tf::Token("physics:localRot1"); }
inline freeusd::tf::Token physics_lowerLimit() { return freeusd::tf::Token("physics:lowerLimit"); }
inline freeusd::tf::Token physics_mass() { return freeusd::tf::Token("physics:mass"); }
inline freeusd::tf::Token physics_maxDistance() { return freeusd::tf::Token("physics:maxDistance"); }
inline freeusd::tf::Token physics_mergeGroup() { return freeusd::tf::Token("physics:mergeGroup"); }
inline freeusd::tf::Token physics_minDistance() { return freeusd::tf::Token("physics:minDistance"); }
inline freeusd::tf::Token physics_principalAxes() { return freeusd::tf::Token("physics:principalAxes"); }
inline freeusd::tf::Token physics_restitution() { return freeusd::tf::Token("physics:restitution"); }
inline freeusd::tf::Token physics_rigidBodyEnabled() { return freeusd::tf::Token("physics:rigidBodyEnabled"); }
inline freeusd::tf::Token physics_simulationOwner() { return freeusd::tf::Token("physics:simulationOwner"); }
inline freeusd::tf::Token physics_startsAsleep() { return freeusd::tf::Token("physics:startsAsleep"); }
inline freeusd::tf::Token physics_staticFriction() { return freeusd::tf::Token("physics:staticFriction"); }
inline freeusd::tf::Token physics_upperLimit() { return freeusd::tf::Token("physics:upperLimit"); }
inline freeusd::tf::Token physics_velocity() { return freeusd::tf::Token("physics:velocity"); }
inline freeusd::tf::Token proxyPrim() { return freeusd::tf::Token("proxyPrim"); }
inline freeusd::tf::Token purpose() { return freeusd::tf::Token("purpose"); }
inline freeusd::tf::Token visibility() { return freeusd::tf::Token("visibility"); }

}  // namespace freeusd::usdPhysics::tokens
