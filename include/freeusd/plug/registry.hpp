#pragma once

#include <string>
#include <vector>

namespace freeusd::plug {

/// Plugin discovery hook (PlugRegistry-shaped). Paths are recorded; dynamic ``.so`` loading is not implemented.
class Registry {
 public:
  static Registry& Get() noexcept;

  /// Append plugin search paths (duplicates are skipped).
  void RegisterPluginPaths(const std::vector<std::string>& paths);
  /// Recorded paths from @ref RegisterPluginPaths (stable order, first registration wins per path string).
  const std::vector<std::string>& RegisteredPluginPaths() const noexcept { return paths_; }
  /// No-op today (no dlopen); reserved for future plugin modules.
  void LoadAllPlugins();

 private:
  Registry() = default;

  std::vector<std::string> paths_;
};

}  // namespace freeusd::plug
