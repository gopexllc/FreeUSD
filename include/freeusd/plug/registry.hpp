#pragma once

#include <mutex>
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
  /// Thread-safe snapshot of @ref RegisteredPluginPaths for concurrent validation.
  std::vector<std::string> SnapshotRegisteredPluginPaths() const;
  /// No-op today (no dlopen); reserved for future plugin modules.
  void LoadAllPlugins();
  /// Count of successful @ref LoadAllPlugins calls on this process singleton.
  std::size_t LoadPassCount() const;

 private:
  Registry() = default;

  mutable std::mutex mu_;
  std::vector<std::string> paths_;
  std::size_t load_pass_count_{0};
};

}  // namespace freeusd::plug
