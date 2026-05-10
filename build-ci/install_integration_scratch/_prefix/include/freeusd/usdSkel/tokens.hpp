#pragma once

#include "freeusd/tf/token.hpp"

namespace freeusd::usdSkel::tokens {

inline freeusd::tf::Token Skeleton() { return freeusd::tf::Token("Skeleton"); }
inline freeusd::tf::Token SkelRoot() { return freeusd::tf::Token("SkelRoot"); }
inline freeusd::tf::Token SkelAnimation() { return freeusd::tf::Token("SkelAnimation"); }

}  // namespace freeusd::usdSkel::tokens
