#include "freeusd/ar/pathSecurity.hpp"

#include <algorithm>
#include <filesystem>

namespace freeusd::ar {

namespace fs = std::filesystem;

std::string CanonicalizeFilesystemPath(const std::string& path) {
  if (path.empty()) {
    return {};
  }
  std::error_code ec;
  const fs::path canon = fs::weakly_canonical(fs::path{path}, ec);
  if (ec) {
    return {};
  }
  return canon.generic_string();
}

bool PathIsWithinRootDirectory(const std::string& resolved_canonical, const std::string& root_canonical) {
  if (resolved_canonical.empty() || root_canonical.empty()) {
    return false;
  }
  std::string root = root_canonical;
  std::string resolved = resolved_canonical;
  std::replace(root.begin(), root.end(), '\\', '/');
  std::replace(resolved.begin(), resolved.end(), '\\', '/');
  if (root.back() != '/') {
    root.push_back('/');
  }
  if (resolved == root_canonical || resolved + '/' == root) {
    return true;
  }
  if (resolved.size() < root.size()) {
    return false;
  }
  return resolved.compare(0, root.size(), root) == 0;
}

std::string ResolvePathUnderAnchor(const std::string& anchor_directory, const std::string& authored_path) {
  if (authored_path.empty() || anchor_directory.empty()) {
    return {};
  }
  fs::path authored{authored_path};
  const std::string anchor_canon = CanonicalizeFilesystemPath(anchor_directory);
  if (anchor_canon.empty()) {
    return {};
  }
  fs::path resolved;
  if (authored.is_absolute()) {
    resolved = fs::path{authored};
  } else {
    resolved = fs::path{anchor_canon} / authored;
  }
  const std::string canon = CanonicalizeFilesystemPath(resolved.string());
  if (canon.empty()) {
    return {};
  }
  // Security: reject paths that escape the layer/asset anchor (e.g. ``../`` outside the package root).
  if (!PathIsWithinRootDirectory(canon, anchor_canon)) {
    return {};
  }
  return canon;
}

}  // namespace freeusd::ar
