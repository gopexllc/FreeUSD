#include "freeusd/ar/defaultResolver.hpp"

#include "freeusd/ar/pathSecurity.hpp"

namespace freeusd::ar {

DefaultResolver::DefaultResolver(std::string anchor_directory)
    : anchor_(CanonicalizeFilesystemPath(std::move(anchor_directory))) {}

std::string DefaultResolver::Resolve(std::string_view asset_path) {
  if (asset_path.empty()) {
    return {};
  }
  if (!anchor_.empty()) {
    return ResolvePathUnderAnchor(anchor_, std::string{asset_path});
  }
  return CanonicalizeFilesystemPath(std::string{asset_path});
}

}  // namespace freeusd::ar
