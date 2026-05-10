#pragma once

#include <string>
#include <utility>

namespace freeusd::ar {

/// Path string produced by a resolver (ArResolvedPath-shaped role; clean-room).
/// Distinct from a raw authored asset path for API clarity; **`DefaultResolver::Resolve`** still returns **`std::string`** today.
struct ResolvedPath {
  std::string path{};

  bool IsEmpty() const noexcept { return path.empty(); }
  const std::string& GetPath() const noexcept { return path; }
  void SetPath(std::string p) { path = std::move(p); }

  bool operator==(const ResolvedPath& o) const noexcept { return path == o.path; }
  bool operator!=(const ResolvedPath& o) const noexcept { return path != o.path; }
};

}  // namespace freeusd::ar
