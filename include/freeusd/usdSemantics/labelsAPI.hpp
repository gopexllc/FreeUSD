#pragma once

#include <memory>
#include <string>
#include <vector>

#include "freeusd/export.hpp"
#include "freeusd/sdf/path.hpp"
#include "freeusd/usd/prim.hpp"

namespace freeusd::usd {
class Stage;
}

namespace freeusd::usdSemantics {

/// Multiple-apply semantic labels helper for authored ``semantics:labels:<instance>`` token arrays.
struct FREEUSD_API SemanticLabelsAPI {
  freeusd::usd::Prim prim;

  SemanticLabelsAPI() = default;
  explicit SemanticLabelsAPI(freeusd::usd::Prim p) : prim(std::move(p)) {}

  static SemanticLabelsAPI ReadFromPrim(const std::shared_ptr<const freeusd::usd::Stage>& stage,
                                        const freeusd::sdf::Path& path);

  explicit operator bool() const noexcept { return prim.IsValid(); }

  bool HasLabels(const std::string& instance_name) const;
  std::vector<std::string> ListLabelSetNames() const;
  std::vector<std::string> GetLabels(const std::string& instance_name, double time = 1.0) const;
};

}  // namespace freeusd::usdSemantics
