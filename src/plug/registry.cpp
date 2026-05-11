#include "freeusd/plug/registry.hpp"

#include <unordered_set>

namespace freeusd::plug {

Registry& Registry::Get() noexcept {
  static Registry r;
  return r;
}

void Registry::RegisterPluginPaths(const std::vector<std::string>& paths) {
  std::lock_guard<std::mutex> lock(mu_);
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

std::vector<std::string> Registry::SnapshotRegisteredPluginPaths() const {
  std::lock_guard<std::mutex> lock(mu_);
  return paths_;
}

void Registry::LoadAllPlugins() {
  std::lock_guard<std::mutex> lock(mu_);
  ++load_pass_count_;
}

std::size_t Registry::LoadPassCount() const {
  std::lock_guard<std::mutex> lock(mu_);
  return load_pass_count_;
}

}  // namespace freeusd::plug
