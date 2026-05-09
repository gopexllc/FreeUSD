#include "freeusd/value.hpp"

#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace freeusd {

Value::Value() = default;

Value Value::empty() {
  return Value();
}

Value Value::boolean(bool value) {
  Value result;
  result.kind_ = ValueKind::Bool;
  result.storage_ = value;
  return result;
}

Value Value::integer(std::int64_t value) {
  Value result;
  result.kind_ = ValueKind::Int;
  result.storage_ = value;
  return result;
}

Value Value::number(double value) {
  Value result;
  result.kind_ = ValueKind::Double;
  result.storage_ = value;
  return result;
}

Value Value::string(std::string value) {
  Value result;
  result.kind_ = ValueKind::String;
  result.storage_ = std::move(value);
  return result;
}

Value Value::token(std::string value) {
  Value result;
  result.kind_ = ValueKind::Token;
  result.storage_ = std::move(value);
  return result;
}

bool Value::as_bool() const {
  if (kind_ != ValueKind::Bool) {
    throw std::logic_error("FreeUSD value is not a bool");
  }
  return std::get<bool>(storage_);
}

std::int64_t Value::as_int() const {
  if (kind_ != ValueKind::Int) {
    throw std::logic_error("FreeUSD value is not an int");
  }
  return std::get<std::int64_t>(storage_);
}

double Value::as_double() const {
  if (kind_ == ValueKind::Int) {
    return static_cast<double>(std::get<std::int64_t>(storage_));
  }
  if (kind_ != ValueKind::Double) {
    throw std::logic_error("FreeUSD value is not a number");
  }
  return std::get<double>(storage_);
}

const std::string& Value::as_string() const {
  if (kind_ != ValueKind::String && kind_ != ValueKind::Token) {
    throw std::logic_error("FreeUSD value is not a string-like value");
  }
  return std::get<std::string>(storage_);
}

std::string Value::repr() const {
  switch (kind_) {
    case ValueKind::Empty:
      return "<empty>";
    case ValueKind::Bool:
      return as_bool() ? "true" : "false";
    case ValueKind::Int:
      return std::to_string(as_int());
    case ValueKind::Double: {
      std::ostringstream stream;
      stream << std::setprecision(17) << std::get<double>(storage_);
      return stream.str();
    }
    case ValueKind::String:
      return "\"" + as_string() + "\"";
    case ValueKind::Token:
      return as_string();
  }
  return "<empty>";
}

const char* to_string(ValueKind kind) {
  switch (kind) {
    case ValueKind::Empty:
      return "empty";
    case ValueKind::Bool:
      return "bool";
    case ValueKind::Int:
      return "int";
    case ValueKind::Double:
      return "double";
    case ValueKind::String:
      return "string";
    case ValueKind::Token:
      return "token";
  }
  return "empty";
}

}  // namespace freeusd
