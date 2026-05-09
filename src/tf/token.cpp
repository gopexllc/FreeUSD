#include "freeusd/tf/token.hpp"

#include <mutex>
#include <unordered_map>
#include <vector>

namespace freeusd::tf {
namespace {

std::mutex& pool_mutex() {
  static std::mutex m;
  return m;
}

// id 0 reserved for empty; index in table is id-1.
std::vector<std::string>& token_table() {
  static std::vector<std::string> t;
  return t;
}

std::unordered_map<std::string, std::uint64_t>& token_lookup() {
  static std::unordered_map<std::string, std::uint64_t> m;
  return m;
}

}  // namespace

Token::Token(std::string_view text) {
  if (text.empty()) {
    return;
  }
  std::string key{text};
  std::lock_guard<std::mutex> lock(pool_mutex());
  auto& tab = token_table();
  auto& look = token_lookup();
  auto it = look.find(key);
  if (it != look.end()) {
    id_ = it->second;
    return;
  }
  const std::uint64_t next = static_cast<std::uint64_t>(tab.size()) + 1U;
  tab.push_back(std::move(key));
  id_ = next;
  look.emplace(tab.back(), next);
}

const std::string& Token::GetText() const {
  static const std::string kEmpty;
  if (id_ == 0) {
    return kEmpty;
  }
  std::lock_guard<std::mutex> lock(pool_mutex());
  const auto& tab = token_table();
  const std::size_t idx = static_cast<std::size_t>(id_ - 1U);
  if (idx >= tab.size()) {
    return kEmpty;
  }
  return tab[idx];
}

const char* Token::GetCStr() const {
  return GetText().c_str();
}

bool operator<(const Token& a, const Token& b) noexcept { return a.GetText() < b.GetText(); }

}  // namespace freeusd::tf
