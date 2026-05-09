#include "freeusd/tf/hash.hpp"

#include <cstdint>

namespace freeusd::tf {

// FNV-1a 64-bit
std::size_t HashString::operator()(const std::string& s) const noexcept {
  std::uint64_t h = 14695981039346656037ULL;
  for (unsigned char c : s) {
    h ^= static_cast<std::uint64_t>(c);
    h *= 1099511628211ULL;
  }
  return static_cast<std::size_t>(h);
}

}  // namespace freeusd::tf
