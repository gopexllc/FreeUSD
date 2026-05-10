#include "freeusd/vt/value.hpp"

#include <sstream>
#include <type_traits>
#include <vector>

#include "freeusd/gf/quatf.hpp"

namespace freeusd::vt {

Value Value::MakeBool(bool v) { return Value{ValuePayload{v}}; }
Value Value::MakeInt32(std::int32_t v) { return Value{ValuePayload{v}}; }
Value Value::MakeInt64(std::int64_t v) { return Value{ValuePayload{v}}; }
Value Value::MakeFloat(float v) { return Value{ValuePayload{v}}; }
Value Value::MakeDouble(double v) { return Value{ValuePayload{v}}; }
Value Value::MakeString(std::string v) { return Value{ValuePayload{std::move(v)}}; }
Value Value::MakeToken(freeusd::tf::Token v) { return Value{ValuePayload{std::move(v)}}; }
Value Value::MakeTokenArray(std::vector<freeusd::tf::Token> v) { return Value{ValuePayload{std::move(v)}}; }
Value Value::MakeQuatd(freeusd::gf::Quatd v) { return Value{ValuePayload{v}}; }
Value Value::MakeQuatf(freeusd::gf::Quatf v) { return Value{ValuePayload{v}}; }
Value Value::MakeMatrix4d(freeusd::gf::Matrix4d v) { return Value{ValuePayload{v}}; }
Value Value::MakeVec3d(freeusd::gf::Vec3d v) { return Value{ValuePayload{v}}; }
Value Value::MakeVec3f(freeusd::gf::Vec3f v) { return Value{ValuePayload{v}}; }

bool Value::IsEmpty() const noexcept { return std::holds_alternative<std::monostate>(payload_); }

bool Value::HoldsBool() const noexcept { return std::holds_alternative<bool>(payload_); }
bool Value::HoldsInt32() const noexcept { return std::holds_alternative<std::int32_t>(payload_); }
bool Value::HoldsInt64() const noexcept { return std::holds_alternative<std::int64_t>(payload_); }
bool Value::HoldsFloat() const noexcept { return std::holds_alternative<float>(payload_); }
bool Value::HoldsDouble() const noexcept { return std::holds_alternative<double>(payload_); }
bool Value::HoldsString() const noexcept { return std::holds_alternative<std::string>(payload_); }
bool Value::HoldsToken() const noexcept { return std::holds_alternative<freeusd::tf::Token>(payload_); }
bool Value::HoldsTokenArray() const noexcept {
  return std::holds_alternative<std::vector<freeusd::tf::Token>>(payload_);
}
bool Value::HoldsQuatd() const noexcept { return std::holds_alternative<freeusd::gf::Quatd>(payload_); }
bool Value::HoldsQuatf() const noexcept { return std::holds_alternative<freeusd::gf::Quatf>(payload_); }
bool Value::HoldsMatrix4d() const noexcept { return std::holds_alternative<freeusd::gf::Matrix4d>(payload_); }
bool Value::HoldsVec3d() const noexcept { return std::holds_alternative<freeusd::gf::Vec3d>(payload_); }
bool Value::HoldsVec3f() const noexcept { return std::holds_alternative<freeusd::gf::Vec3f>(payload_); }

bool Value::GetBool(bool* out) const noexcept {
  if (!out || !HoldsBool()) {
    return false;
  }
  *out = std::get<bool>(payload_);
  return true;
}

bool Value::GetInt32(std::int32_t* out) const noexcept {
  if (!out || !HoldsInt32()) {
    return false;
  }
  *out = std::get<std::int32_t>(payload_);
  return true;
}

bool Value::GetInt64(std::int64_t* out) const noexcept {
  if (!out || !HoldsInt64()) {
    return false;
  }
  *out = std::get<std::int64_t>(payload_);
  return true;
}

bool Value::GetFloat(float* out) const noexcept {
  if (!out || !HoldsFloat()) {
    return false;
  }
  *out = std::get<float>(payload_);
  return true;
}

bool Value::GetDouble(double* out) const noexcept {
  if (!out) {
    return false;
  }
  if (const auto* d = std::get_if<double>(&payload_)) {
    *out = *d;
    return true;
  }
  if (const auto* f = std::get_if<float>(&payload_)) {
    *out = static_cast<double>(*f);
    return true;
  }
  if (const auto* i = std::get_if<std::int32_t>(&payload_)) {
    *out = static_cast<double>(*i);
    return true;
  }
  if (const auto* l = std::get_if<std::int64_t>(&payload_)) {
    *out = static_cast<double>(*l);
    return true;
  }
  return false;
}

bool Value::GetString(std::string* out) const {
  if (!out || !HoldsString()) {
    return false;
  }
  *out = std::get<std::string>(payload_);
  return true;
}

bool Value::GetToken(freeusd::tf::Token* out) const {
  if (!out || !HoldsToken()) {
    return false;
  }
  *out = std::get<freeusd::tf::Token>(payload_);
  return true;
}

bool Value::GetTokenArray(std::vector<freeusd::tf::Token>* out) const {
  if (!out || !HoldsTokenArray()) {
    return false;
  }
  *out = std::get<std::vector<freeusd::tf::Token>>(payload_);
  return true;
}

bool Value::GetQuatd(freeusd::gf::Quatd* out) const noexcept {
  if (!out || !HoldsQuatd()) {
    return false;
  }
  *out = std::get<freeusd::gf::Quatd>(payload_);
  return true;
}

bool Value::GetQuatf(freeusd::gf::Quatf* out) const noexcept {
  if (!out || !HoldsQuatf()) {
    return false;
  }
  *out = std::get<freeusd::gf::Quatf>(payload_);
  return true;
}

bool Value::GetMatrix4d(freeusd::gf::Matrix4d* out) const noexcept {
  if (!out || !HoldsMatrix4d()) {
    return false;
  }
  *out = std::get<freeusd::gf::Matrix4d>(payload_);
  return true;
}

bool Value::GetVec3d(freeusd::gf::Vec3d* out) const noexcept {
  if (!out || !HoldsVec3d()) {
    return false;
  }
  *out = std::get<freeusd::gf::Vec3d>(payload_);
  return true;
}

bool Value::GetVec3f(freeusd::gf::Vec3f* out) const noexcept {
  if (!out || !HoldsVec3f()) {
    return false;
  }
  *out = std::get<freeusd::gf::Vec3f>(payload_);
  return true;
}

std::ostream& operator<<(std::ostream& os, const Value& v) {
  if (v.IsEmpty()) {
    return os << "<empty>";
  }
  std::visit(
      [&](auto&& arg) {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, std::monostate>) {
          os << "<empty>";
        } else if constexpr (std::is_same_v<T, bool>) {
          os << (arg ? "true" : "false");
        } else if constexpr (std::is_same_v<T, freeusd::tf::Token>) {
          os << arg.GetText();
        } else if constexpr (std::is_same_v<T, std::vector<freeusd::tf::Token>>) {
          os << '[';
          for (std::size_t i = 0; i < arg.size(); ++i) {
            if (i) {
              os << ", ";
            }
            os << '@' << arg[i].GetText();
          }
          os << ']';
        } else if constexpr (std::is_same_v<T, freeusd::gf::Quatd>) {
          os << '(' << arg.real << "," << arg.i << "," << arg.j << "," << arg.k << ')';
        } else if constexpr (std::is_same_v<T, freeusd::gf::Quatf>) {
          os << '(' << arg.real << "," << arg.i << "," << arg.j << "," << arg.k << ')';
        } else if constexpr (std::is_same_v<T, freeusd::gf::Matrix4d>) {
          os << "((";
          for (int row = 0; row < 4; ++row) {
            if (row) {
              os << "), (";
            }
            for (int col = 0; col < 4; ++col) {
              if (col) {
                os << ", ";
              }
              os << arg.m[static_cast<std::size_t>(row * 4 + col)];
            }
          }
          os << "))";
        } else if constexpr (std::is_same_v<T, freeusd::gf::Vec3d>) {
          os << "(" << arg.x() << "," << arg.y() << "," << arg.z() << ")";
        } else if constexpr (std::is_same_v<T, freeusd::gf::Vec3f>) {
          os << "(" << arg.x() << "," << arg.y() << "," << arg.z() << ")";
        } else {
          os << arg;
        }
      },
      v.GetPayload());
  return os;
}

}  // namespace freeusd::vt
