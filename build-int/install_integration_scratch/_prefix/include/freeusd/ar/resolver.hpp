#pragma once

#include <string>
#include <string_view>

#include "freeusd/export.hpp"

namespace freeusd::ar {

/// Asset resolution interface (ArResolver role, clean-room).
class FREEUSD_API Resolver {
 public:
  virtual ~Resolver() = default;
  virtual std::string Resolve(std::string_view asset_path) = 0;
};

}  // namespace freeusd::ar
