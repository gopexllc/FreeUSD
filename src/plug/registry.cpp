#include "freeusd/plug/registry.hpp"

#include <unordered_set>

namespace freeusd::plug {

Registry& Registry::Get() noexcept {
  static Registry r;
  return r;
}

void Registry::RegisterPluginPaths(const std::vector<std::string>& paths) {
  std::unordered_set<std::string> seen(paths_.begin(), paths_.end());
  for (const std::string& p : paths) {
    if (p.empty()) {
      continue;
    }
    if (seen.insert(p).second) {
      paths_.push_back(p);
    }
  }
}

void Registry::LoadAllPlugins() {}

}  // namespace freeusd::plug
