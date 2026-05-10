#pragma once

#include <memory>

#include "freeusd/sdf/layer.hpp"

namespace freeusd::usd {

/// Layer (or root) an edit is directed at (UsdEditTarget-shaped role; clean-room).
struct EditTarget {
  std::shared_ptr<freeusd::sdf::Layer> layer;

  bool IsValid() const noexcept { return static_cast<bool>(layer); }
};

}  // namespace freeusd::usd
