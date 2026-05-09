#include "freeusd/ar/defaultResolver.hpp"

#include <filesystem>

namespace freeusd::ar {

namespace fs = std::filesystem;

DefaultResolver::DefaultResolver(std::string anchor_directory) : anchor_(std::move(anchor_directory)) {}

std::string DefaultResolver::Resolve(std::string_view asset_path) {
  if (asset_path.empty()) {
    return {};
  }
  fs::path p{std::string{asset_path}};
  if (p.is_absolute()) {
    return fs::weakly_canonical(p).string();
  }
  if (anchor_.empty()) {
    return fs::weakly_canonical(fs::absolute(p)).string();
  }
  const fs::path base{anchor_};
  return fs::weakly_canonical(base / p).string();
}

}  // namespace freeusd::ar
