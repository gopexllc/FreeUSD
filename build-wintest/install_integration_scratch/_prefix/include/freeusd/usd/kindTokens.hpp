#pragma once

#include "freeusd/tf/token.hpp"

namespace freeusd::usd::kindTokens {

/// Prim **`kind`** metadata values (UsdKindTokens-shaped names; clean-room).
/// These are the usual model-hierarchy tokens authored on prims as `kind = "…"` in USDA.
inline freeusd::tf::Token Component() { return freeusd::tf::Token("component"); }
inline freeusd::tf::Token Group() { return freeusd::tf::Token("group"); }
inline freeusd::tf::Token Assembly() { return freeusd::tf::Token("assembly"); }
inline freeusd::tf::Token Subcomponent() { return freeusd::tf::Token("subcomponent"); }

}  // namespace freeusd::usd::kindTokens
