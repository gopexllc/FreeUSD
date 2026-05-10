#pragma once

#include <cstddef>
#include <cstdint>

namespace freeusd {

struct SourceLocation {
  std::size_t byte_offset{0};
  std::uint32_t line{1};
  std::uint32_t column{1};
};

}  // namespace freeusd
