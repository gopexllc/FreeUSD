#pragma once

#include "freeusd/tf/token.hpp"

namespace freeusd::usd::tokens {

/// Common prim composition list field names (UsdTokens-style naming; clean-room).

inline freeusd::tf::Token References() { return freeusd::tf::Token("references"); }
inline freeusd::tf::Token Payload() { return freeusd::tf::Token("payload"); }
inline freeusd::tf::Token Inherits() { return freeusd::tf::Token("inherits"); }
inline freeusd::tf::Token Specializes() { return freeusd::tf::Token("specializes"); }
inline freeusd::tf::Token VariantSets() { return freeusd::tf::Token("variantSets"); }
inline freeusd::tf::Token VariantSelection() { return freeusd::tf::Token("variantSelection"); }
inline freeusd::tf::Token PrefixSubstitutions() { return freeusd::tf::Token("prefixSubstitutions"); }
inline freeusd::tf::Token Relocates() { return freeusd::tf::Token("relocates"); }

}  // namespace freeusd::usd::tokens
