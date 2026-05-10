#pragma once

#include "freeusd/tf/token.hpp"

namespace freeusd::usdGeom::tokens {

/// Schema tokens (UsdGeomTokens-shaped names; clean-room).
inline freeusd::tf::Token Mesh() { return freeusd::tf::Token("Mesh"); }
inline freeusd::tf::Token Xform() { return freeusd::tf::Token("Xform"); }
inline freeusd::tf::Token Scope() { return freeusd::tf::Token("Scope"); }

}  // namespace freeusd::usdGeom::tokens
