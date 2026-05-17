#include <cassert>
#include <filesystem>
#include <string>

#include "freeusd/ar/pathSecurity.hpp"

namespace fs = std::filesystem;

int main() {
  const fs::path tmp = fs::temp_directory_path() / "freeusd_ar_path_security";
  std::error_code ec;
  fs::create_directories(tmp, ec);
  assert(!ec);

  const std::string anchor = freeusd::ar::CanonicalizeFilesystemPath(tmp.string());
  assert(!anchor.empty());

  const std::string inside = freeusd::ar::ResolvePathUnderAnchor(anchor, "child/file.usda");
  assert(!inside.empty());
  assert(freeusd::ar::PathIsWithinRootDirectory(inside, anchor));

  assert(freeusd::ar::ResolvePathUnderAnchor(anchor, "../outside.usda").empty());
  assert(freeusd::ar::ResolvePathUnderAnchor(anchor, "pkg/../../outside.usda").empty());

  const std::string nested = freeusd::ar::ResolvePathUnderAnchor(anchor, "a/b/c.usda");
  assert(!nested.empty());
  assert(freeusd::ar::PathIsWithinRootDirectory(nested, anchor));

  return 0;
}
