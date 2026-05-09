#pragma once

#include <memory>
#include <vector>

#include "freeusd/export.hpp"

namespace freeusd::sdf {
class Layer;
class Path;
}  // namespace freeusd::sdf
namespace freeusd::tf {
class Token;
}
namespace freeusd::vt {
class Value;
}

namespace freeusd::pcp {

/// Walk \p strongest_first from strongest (index \c 0) to weakest; first layer whose
/// `GetFieldAtTime` succeeds wins (OpenUSD-ish falloff for default + time samples only).
FREEUSD_API bool ResolveFieldAtTimeStrongestWins(const std::vector<std::shared_ptr<freeusd::sdf::Layer>>& strongest_first,
                                                 const freeusd::sdf::Path& prim_path,
                                                 const freeusd::tf::Token& name,
                                                 double time,
                                                 freeusd::vt::Value* out);

FREEUSD_API bool HasFieldOpinionOnAnyLayer(const std::vector<std::shared_ptr<freeusd::sdf::Layer>>& strongest_first,
                                           const freeusd::sdf::Path& prim_path,
                                           const freeusd::tf::Token& name);

}  // namespace freeusd::pcp
