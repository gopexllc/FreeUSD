#pragma once

#include <functional>
#include <memory>

#include "freeusd/export.hpp"
#include "freeusd/pcp/layerStack.hpp"
#include "freeusd/sdf/layer.hpp"

namespace freeusd::pcp {

/// Build a trivial layer stack: \p root first (strongest), then authored \c subLayers in array order
/// (first sublayer is stronger than subsequent ones; matches typical USD authoring).
///
/// Missing layers (callable returns null) are skipped. Does **not** recurse into sublayers of
/// resolved layers—that needs a fuller Pcp graph walk.
FREEUSD_API LayerStack ComposeSublayers(
    const std::shared_ptr<freeusd::sdf::Layer>& root,
    const std::function<std::shared_ptr<freeusd::sdf::Layer>(const std::string& authored_path_or_id)>&
        resolve_layer);

/// Depth-first expansion: emit \p root, then recurse each authored sublayer in order before moving to sibling
/// sublayers. Uses a recursion stack keyed by resolved layer pointers to suppress infinite regress on cyclic
/// sublayer graphs (skips revisiting layers still “open” upward).
FREEUSD_API LayerStack ComposeSublayersDepthFirst(
    const std::shared_ptr<freeusd::sdf::Layer>& root,
    const std::function<std::shared_ptr<freeusd::sdf::Layer>(const std::string& authored_path_or_id)>&
        resolve_layer);

}  // namespace freeusd::pcp
