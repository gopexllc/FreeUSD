#pragma once

#include "freeusd/tf/token.hpp"

namespace freeusd::usdShade::tokens {

/// Schema-oriented tokens (UsdShade-shaped names; clean-room, no upstream codegen).
inline freeusd::tf::Token Material() { return freeusd::tf::Token("Material"); }
inline freeusd::tf::Token Shader() { return freeusd::tf::Token("Shader"); }
inline freeusd::tf::Token NodeGraph() { return freeusd::tf::Token("NodeGraph"); }

}  // namespace freeusd::usdShade::tokens
