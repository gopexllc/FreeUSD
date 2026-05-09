#pragma once

#include <memory>
#include <vector>

#include "freeusd/export.hpp"
#include "freeusd/sdf/layer.hpp"

namespace freeusd::pcp {

/// Ordered layer stack (PcpLayerStack-shaped; composition rules are intentionally minimal).
class FREEUSD_API LayerStack {
 public:
  void Append(std::shared_ptr<freeusd::sdf::Layer> layer);
  const std::vector<std::shared_ptr<freeusd::sdf::Layer>>& GetLayers() const noexcept { return layers_; }
  bool IsEmpty() const noexcept { return layers_.empty(); }

 private:
  std::vector<std::shared_ptr<freeusd::sdf::Layer>> layers_;
};

}  // namespace freeusd::pcp
