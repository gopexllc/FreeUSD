#pragma once

#include "freeusd/tf/token.hpp"

namespace freeusd::usdRi::tokens {

/// Ri / RIS integration tokens (naming aligned with common USD usage; clean-room).
inline freeusd::tf::Token RisObject() { return freeusd::tf::Token("RisObject"); }

}  // namespace freeusd::usdRi::tokens
