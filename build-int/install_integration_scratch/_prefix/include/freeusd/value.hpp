#pragma once

#include <cstdint>
#include <string>
#include <variant>

#include "freeusd/export.hpp"

namespace freeusd {

enum class ValueKind {
  Empty,
  Bool,
  Int,
  Double,
  String,
  Token,
};

class FREEUSD_API Value {
 public:
  Value();

  static Value empty();
  static Value boolean(bool value);
  static Value integer(std::int64_t value);
  static Value number(double value);
  static Value string(std::string value);
  static Value token(std::string value);

  ValueKind kind() const noexcept { return kind_; }
  bool is_empty() const noexcept { return kind_ == ValueKind::Empty; }

  bool as_bool() const;
  std::int64_t as_int() const;
  double as_double() const;
  const std::string& as_string() const;

  std::string repr() const;

 private:
  using Storage = std::variant<std::monostate, bool, std::int64_t, double, std::string>;

  ValueKind kind_{ValueKind::Empty};
  Storage storage_{std::monostate{}};
};

FREEUSD_API const char* to_string(ValueKind kind);

}  // namespace freeusd
