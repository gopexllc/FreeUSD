#pragma once

namespace freeusd::sdf {

/// Per-layer time mapping used in composition discussions (SdfLayerOffset-shaped role; clean-room).
struct LayerOffset {
  double offset{0.0};
  double scale{1.0};

  constexpr bool IsIdentity() const noexcept { return offset == 0.0 && scale == 1.0; }
};

}  // namespace freeusd::sdf
