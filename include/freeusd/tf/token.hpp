#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <string_view>

#include "freeusd/export.hpp"

namespace freeusd::tf {

/// Interned identifier (OpenUSD-style TfToken, clean-room).
class FREEUSD_API Token {
 public:
  Token() = default;
  explicit Token(std::string_view text);

  bool IsEmpty() const noexcept { return id_ == 0; }
  const std::string& GetText() const;
  const char* GetCStr() const;

  bool operator==(const Token& o) const noexcept { return id_ == o.id_; }
  bool operator!=(const Token& o) const noexcept { return id_ != o.id_; }

  struct Hash {
    std::size_t operator()(const Token& t) const noexcept { return static_cast<std::size_t>(t.id_); }
  };

 private:
  std::uint64_t id_{0};
};

FREEUSD_API bool operator<(const Token& a, const Token& b) noexcept;

}  // namespace freeusd::tf

namespace std {
template <>
struct hash<freeusd::tf::Token> {
  std::size_t operator()(const freeusd::tf::Token& t) const noexcept { return freeusd::tf::Token::Hash{}(t); }
};
}  // namespace std
