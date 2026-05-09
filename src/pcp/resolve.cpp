#include "freeusd/pcp/resolve.hpp"

#include "freeusd/sdf/layer.hpp"
#include "freeusd/tf/token.hpp"
#include "freeusd/vt/value.hpp"

namespace freeusd::pcp {

bool ResolveFieldAtTimeStrongestWins(const std::vector<std::shared_ptr<freeusd::sdf::Layer>>& strongest_first,
                                     const freeusd::sdf::Path& prim_path,
                                     const freeusd::tf::Token& name,
                                     double time,
                                     freeusd::vt::Value* out) {
  if (!out || name.IsEmpty()) {
    return false;
  }
  for (const std::shared_ptr<freeusd::sdf::Layer>& L : strongest_first) {
    if (!L) {
      continue;
    }
    if (L->GetFieldAtTime(prim_path, name, time, out)) {
      return true;
    }
  }
  return false;
}

bool HasFieldOpinionOnAnyLayer(const std::vector<std::shared_ptr<freeusd::sdf::Layer>>& strongest_first,
                               const freeusd::sdf::Path& prim_path,
                               const freeusd::tf::Token& name) {
  if (name.IsEmpty()) {
    return false;
  }
  for (const std::shared_ptr<freeusd::sdf::Layer>& L : strongest_first) {
    if (L && L->HasField(prim_path, name)) {
      return true;
    }
  }
  return false;
}

}  // namespace freeusd::pcp
