#pragma once

#include <memory>

#include "freeusd/export.hpp"

namespace freeusd::sdf {
class Layer;
}

namespace freeusd::usd {
class Stage;
}

/// UsdUtils-shaped public entry points (clean-room). No Pixar / AOUSD sources.
/// Only a narrow time-sampled flatten helper is implemented today; broader collection / stage cache helpers remain future work.

namespace freeusd::usdUtils {

/// Options for a future ``FlattenLayerStack``-style pass (field names mirror common USD discussions).
struct FlattenOptions {
  bool merge_authored_layer_metadata{true};
  bool flatten_contribution_sublayers{false};
  bool preserve_relative_asset_paths{false};
};

/// Compose the current stage view at a single evaluation time into a new in-memory layer. This flattens prim
/// hierarchy, composed relationships, composition list metadata, and evaluated attribute defaults into one layer.
FREEUSD_API std::shared_ptr<freeusd::sdf::Layer> FlattenStageAtTime(const freeusd::usd::Stage& stage, double time = 1.0,
                                                                    const FlattenOptions& options = {});

}  // namespace freeusd::usdUtils
