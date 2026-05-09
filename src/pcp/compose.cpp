#include "freeusd/pcp/compose.hpp"

#include <unordered_set>

namespace freeusd::pcp {

LayerStack ComposeSublayers(
    const std::shared_ptr<freeusd::sdf::Layer>& root,
    const std::function<std::shared_ptr<freeusd::sdf::Layer>(const std::string& authored_path_or_id)>&
        resolve_layer) {
  LayerStack stack;
  if (!root) {
    return stack;
  }
  stack.Append(root);
  if (!resolve_layer) {
    return stack;
  }
  for (const std::string& authored : root->GetSubLayers()) {
    if (auto L = resolve_layer(authored)) {
      stack.Append(std::move(L));
    }
  }
  return stack;
}

LayerStack ComposeSublayersDepthFirst(
    const std::shared_ptr<freeusd::sdf::Layer>& root,
    const std::function<std::shared_ptr<freeusd::sdf::Layer>(const std::string& authored_path_or_id)>&
        resolve_layer) {
  LayerStack stack;
  if (!root) {
    return stack;
  }
  if (!resolve_layer) {
    stack.Append(root);
    return stack;
  }

  std::unordered_set<const freeusd::sdf::Layer*> open;
  std::function<void(const std::shared_ptr<freeusd::sdf::Layer>&)> dfs;

  dfs = [&](const std::shared_ptr<freeusd::sdf::Layer>& node) {
    if (!node) {
      return;
    }
    if (!open.insert(node.get()).second) {
      return;
    }
    stack.Append(node);
    for (const std::string& authored : node->GetSubLayers()) {
      dfs(resolve_layer(authored));
    }
    open.erase(node.get());
  };

  dfs(root);
  return stack;
}

}  // namespace freeusd::pcp
