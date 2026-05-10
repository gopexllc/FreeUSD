#pragma once

#include <cstddef>
#include <functional>
#include <string>

namespace freeusd::tf {

/// Stable string hash for containers (FNV-1a 64-bit).
struct HashString {
  std::size_t operator()(const std::string& s) const noexcept;
};

}  // namespace freeusd::tf
