#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

#include "freeusd/export.hpp"
#include "freeusd/tf/token.hpp"

namespace freeusd::sdf {

/// Absolute scene path with OpenUSD-like prim/property split (clean-room).
class FREEUSD_API Path {
 public:
  Path() = default;

  /// Parse an absolute path. On failure, yields an empty path.
  static Path FromString(std::string_view text);

  static Path AbsoluteRootPath();

  bool IsEmpty() const noexcept { return text_.empty(); }
  bool IsAbsolutePath() const noexcept { return !text_.empty() && text_[0] == '/'; }
  bool IsAbsoluteRootPath() const noexcept { return text_ == "/"; }

  bool IsPrimPath() const noexcept;
  bool IsPropertyPath() const noexcept;
  bool IsPrimPropertyPath() const noexcept { return IsPropertyPath(); }

  std::size_t GetPathElementCount() const;

  std::string GetText() const { return text_; }
  const std::string& GetString() const noexcept { return text_; }

  Path GetParentPath() const;
  Path GetPrimPath() const;

  /// Final path element: prim name or full property name (e.g. `xformOp:translate`).
  std::string GetName() const;

  bool HasPrefix(const Path& prefix) const;

  Path AppendChild(const freeusd::tf::Token& name) const;
  Path AppendChild(std::string_view name) const;

  bool operator==(const Path& o) const noexcept { return text_ == o.text_; }
  bool operator!=(const Path& o) const noexcept { return text_ != o.text_; }

  struct Hash {
    std::size_t operator()(const Path& p) const noexcept {
      return std::hash<std::string>{}(p.text_);
    }
  };

 private:
  explicit Path(std::string normalized) : text_(std::move(normalized)) {}

  std::string text_;
};

FREEUSD_API std::vector<Path> PathGetPrefixes(const Path& path);

}  // namespace freeusd::sdf
