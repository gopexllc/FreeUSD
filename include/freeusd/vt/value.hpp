#pragma once

#include <array>
#include <cstdint>
#include <iosfwd>
#include <string>
#include <variant>

#include "freeusd/export.hpp"
#include "freeusd/gf/vec3d.hpp"
#include "freeusd/tf/token.hpp"

namespace freeusd::vt {

using ValuePayload = std::variant<std::monostate,
                                  bool,
                                  std::int32_t,
                                  std::int64_t,
                                  float,
                                  double,
                                  std::string,
                                  freeusd::tf::Token,
                                  freeusd::gf::Vec3d>;

/// Typed value bag analogous in role to VtValue (clean-room).
class FREEUSD_API Value {
 public:
  Value() = default;

  static Value MakeBool(bool v);
  static Value MakeInt32(std::int32_t v);
  static Value MakeInt64(std::int64_t v);
  static Value MakeFloat(float v);
  static Value MakeDouble(double v);
  static Value MakeString(std::string v);
  static Value MakeToken(freeusd::tf::Token v);
  static Value MakeVec3d(freeusd::gf::Vec3d v);

  bool IsEmpty() const noexcept;
  bool HoldsBool() const noexcept;
  bool HoldsDouble() const noexcept;
  bool HoldsString() const noexcept;
  bool HoldsToken() const noexcept;
  bool HoldsVec3d() const noexcept;

  bool GetBool(bool* out) const noexcept;
  bool GetDouble(double* out) const noexcept;
  bool GetString(std::string* out) const;
  bool GetToken(freeusd::tf::Token* out) const;
  bool GetVec3d(freeusd::gf::Vec3d* out) const noexcept;

  const ValuePayload& GetPayload() const noexcept { return payload_; }
  ValuePayload& GetPayload() noexcept { return payload_; }

  FREEUSD_API friend std::ostream& operator<<(std::ostream& os, const Value& v);

 private:
  explicit Value(ValuePayload p) : payload_(std::move(p)) {}

  ValuePayload payload_{};
};

}  // namespace freeusd::vt
