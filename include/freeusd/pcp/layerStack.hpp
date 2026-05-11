#pragma once

#include <memory>
#include <vector>

#include "freeusd/export.hpp"
#include "freeusd/sdf/layer.hpp"
#include "freeusd/sdf/layerOffset.hpp"

namespace freeusd::pcp {

struct FREEUSD_API LayerStackEntry {
  std::shared_ptr<freeusd::sdf::Layer> layer;
  freeusd::sdf::LayerOffset offset;
};

/// Ordered layer stack (PcpLayerStack-shaped; composition rules are intentionally minimal).
class FREEUSD_API LayerStack {
 public:
  void Append(std::shared_ptr<freeusd::sdf::Layer> layer, freeusd::sdf::LayerOffset offset = {});
  void Clear() noexcept { layers_.clear(); }
  std::vector<std::shared_ptr<freeusd::sdf::Layer>> GetLayers() const;
  const std::vector<LayerStackEntry>& GetEntries() const noexcept { return layers_; }
  bool IsEmpty() const noexcept { return layers_.empty(); }

 private:
  std::vector<LayerStackEntry> layers_;
};

}  // namespace freeusd::pcp
