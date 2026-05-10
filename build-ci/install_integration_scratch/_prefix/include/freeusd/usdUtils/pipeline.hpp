#pragma once

/// UsdUtils-shaped public entry points (clean-room). No Pixar / AOUSD sources.
/// Full **flatten / collection / stage cache** helpers are not implemented; @ref FlattenOptions carries flags only.

namespace freeusd::usdUtils {

/// Options for a future ``FlattenLayerStack``-style pass (field names mirror common USD discussions).
struct FlattenOptions {
  bool merge_authored_layer_metadata{true};
  bool flatten_contribution_sublayers{false};
  bool preserve_relative_asset_paths{false};
};

}  // namespace freeusd::usdUtils
