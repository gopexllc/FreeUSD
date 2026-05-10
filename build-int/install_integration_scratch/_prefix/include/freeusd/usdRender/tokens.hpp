#pragma once

#include "freeusd/tf/token.hpp"

namespace freeusd::usdRender::tokens {

inline freeusd::tf::Token Settings() { return freeusd::tf::Token("RenderSettings"); }
inline freeusd::tf::Token RenderProduct() { return freeusd::tf::Token("RenderProduct"); }

}  // namespace freeusd::usdRender::tokens
