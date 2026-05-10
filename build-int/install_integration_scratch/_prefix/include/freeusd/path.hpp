#pragma once

#include <optional>
#include <string>
#include <string_view>

#include "freeusd/export.hpp"

namespace freeusd {

class FREEUSD_API Path {
 public:
  Path();
  explicit Path(std::string text);

  static Path root();
  static std::optional<Path> parse(std::string_view text);
  static bool is_valid(std::string_view text);
  static bool is_valid_identifier(std::string_view text);
  static bool is_valid_property_name(std::string_view text);

  const std::string& string() const noexcept { return text_; }
  bool is_root() const noexcept { return text_ == "/"; }
  bool is_absolute() const noexcept { return !text_.empty() && text_[0] == '/'; }
  bool is_property_path() const noexcept;

  std::string name() const;
  std::string parent_string() const;

  Path append_child(std::string_view child_name) const;
  Path append_property(std::string_view property_name) const;

  friend bool operator==(const Path& lhs, const Path& rhs) noexcept { return lhs.text_ == rhs.text_; }
  friend bool operator!=(const Path& lhs, const Path& rhs) noexcept { return !(lhs == rhs); }
  friend bool operator<(const Path& lhs, const Path& rhs) noexcept { return lhs.text_ < rhs.text_; }

 private:
  std::string text_;
};

}  // namespace freeusd
