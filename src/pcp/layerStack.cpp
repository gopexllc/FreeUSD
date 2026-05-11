#include "freeusd/pcp/layerStack.hpp"

namespace freeusd::pcp {

void LayerStack::Append(std::shared_ptr<freeusd::sdf::Layer> layer, freeusd::sdf::LayerOffset offset) {
  if (!layer) {
    return;
  }
  layers_.push_back(LayerStackEntry{std::move(layer), offset});
}

std::vector<std::shared_ptr<freeusd::sdf::Layer>> LayerStack::GetLayers() const {
  std::vector<std::shared_ptr<freeusd::sdf::Layer>> out;
  out.reserve(layers_.size());
  for (const LayerStackEntry& entry : layers_) {
    out.push_back(entry.layer);
  }
  return out;
}

}  // namespace freeusd::pcp
