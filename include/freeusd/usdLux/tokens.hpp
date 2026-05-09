#pragma once

#include "freeusd/tf/token.hpp"

namespace freeusd::usdLux::tokens {

inline freeusd::tf::Token DomeLight() { return freeusd::tf::Token("DomeLight"); }
inline freeusd::tf::Token SphereLight() { return freeusd::tf::Token("SphereLight"); }
inline freeusd::tf::Token DistantLight() { return freeusd::tf::Token("DistantLight"); }

}  // namespace freeusd::usdLux::tokens
