#pragma once

#include <string>
#include <string_view>
#include <utility>

#include "freeusd/diagnostic.hpp"
#include "freeusd/export.hpp"
#include "freeusd/layer.hpp"

namespace freeusd {

struct ParseResult {
  Layer layer;
  Diagnostics diagnostics;

  bool ok() const;
};

FREEUSD_API ParseResult parse_usda(std::string_view source, std::string identifier = {});

}  // namespace freeusd
