#pragma once

#include <string>
#include <utility>

namespace freeusd::sdf {

/// Authored asset identifier string (SdfAssetPath-shaped role; clean-room).
/// Layer I/O continues to use plain `std::string` where historical FreeUSD APIs do; this type is for
/// explicit USD-style typing and Python parity.
struct AssetPath {
  std::string path{};

  bool IsEmpty() const noexcept { return path.empty(); }
  const std::string& GetPath() const noexcept { return path; }
  void SetPath(std::string p) { path = std::move(p); }

  bool operator==(const AssetPath& o) const noexcept { return path == o.path; }
  bool operator!=(const AssetPath& o) const noexcept { return path != o.path; }
};

}  // namespace freeusd::sdf
