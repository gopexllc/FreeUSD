#include "freeusd/pcp/layerStack.hpp"

namespace freeusd::pcp {

void LayerStack::Append(std::shared_ptr<freeusd::sdf::Layer> layer) {
  if (!layer) {
    return;
  }
  layers_.push_back(std::move(layer));
}

}  // namespace freeusd::pcp
