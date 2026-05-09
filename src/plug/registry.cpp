#include "freeusd/plug/registry.hpp"

namespace freeusd::plug {

Registry& Registry::Get() noexcept {
  static Registry r;
  return r;
}

void Registry::RegisterPluginPaths(const std::vector<std::string>&) {}
void Registry::LoadAllPlugins() {}

}  // namespace freeusd::plug
