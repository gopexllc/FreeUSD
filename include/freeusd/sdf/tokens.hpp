#pragma once

#include "freeusd/tf/token.hpp"

namespace freeusd::sdf::tokens {

/// Common layer and prim metadata field names (SdfTokens-style naming; clean-room, not upstream codegen).

inline freeusd::tf::Token SubLayers() { return freeusd::tf::Token("subLayers"); }
inline freeusd::tf::Token DefaultPrim() { return freeusd::tf::Token("defaultPrim"); }
inline freeusd::tf::Token Documentation() { return freeusd::tf::Token("documentation"); }
inline freeusd::tf::Token PrimOrder() { return freeusd::tf::Token("primOrder"); }
inline freeusd::tf::Token MetersPerUnit() { return freeusd::tf::Token("metersPerUnit"); }
inline freeusd::tf::Token UpAxis() { return freeusd::tf::Token("upAxis"); }
inline freeusd::tf::Token StartTimeCode() { return freeusd::tf::Token("startTimeCode"); }
inline freeusd::tf::Token EndTimeCode() { return freeusd::tf::Token("endTimeCode"); }
inline freeusd::tf::Token TimeCodesPerSecond() { return freeusd::tf::Token("timeCodesPerSecond"); }
inline freeusd::tf::Token FramesPerSecond() { return freeusd::tf::Token("framesPerSecond"); }
inline freeusd::tf::Token FramePrecision() { return freeusd::tf::Token("framePrecision"); }
inline freeusd::tf::Token Kind() { return freeusd::tf::Token("kind"); }
inline freeusd::tf::Token Active() { return freeusd::tf::Token("active"); }
inline freeusd::tf::Token CustomData() { return freeusd::tf::Token("customData"); }
inline freeusd::tf::Token VariantSetNames() { return freeusd::tf::Token("variantSetNames"); }

}  // namespace freeusd::sdf::tokens
