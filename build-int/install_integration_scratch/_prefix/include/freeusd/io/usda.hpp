#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <string_view>

#include "freeusd/export.hpp"

namespace freeusd::sdf {
class Layer;
}  // namespace freeusd::sdf

namespace freeusd::io::usda {

struct FREEUSD_API ParseResult {
  bool ok{false};
  std::size_t line{0};
  std::string message;
};

/// Replace all opinions in `layer` with contents parsed from `text` (minimal USDA subset).
FREEUSD_API ParseResult LoadFromString(std::string_view text, const std::shared_ptr<freeusd::sdf::Layer>& layer);

/// Serialize `layer` to a USDA document (minimal subset; stable ordering).
FREEUSD_API std::string SaveToString(const freeusd::sdf::Layer& layer);

FREEUSD_API ParseResult LoadFromFile(const std::string& path, const std::shared_ptr<freeusd::sdf::Layer>& layer);
FREEUSD_API ParseResult SaveToFile(const std::string& path, const freeusd::sdf::Layer& layer);

}  // namespace freeusd::io::usda
