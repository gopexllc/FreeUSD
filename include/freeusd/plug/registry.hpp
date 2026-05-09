#pragma once

#include <string>
#include <vector>

namespace freeusd::plug {

/// Plugin discovery hook (PlugRegistry-shaped; no dynamic loading yet).
class Registry {
 public:
  static Registry& Get() noexcept;

  void RegisterPluginPaths(const std::vector<std::string>& paths);
  void LoadAllPlugins();

 private:
  Registry() = default;
};

}  // namespace freeusd::plug
