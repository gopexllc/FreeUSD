#pragma once

#include <cstdint>
#include <iosfwd>
#include <string>
#include <variant>
#include <vector>

#include "freeusd/export.hpp"
#include "freeusd/gf/matrix4d.hpp"
#include "freeusd/gf/quatd.hpp"
#include "freeusd/gf/quatf.hpp"
#include "freeusd/gf/vec3d.hpp"
#include "freeusd/gf/vec3f.hpp"
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
                                  std::vector<freeusd::tf::Token>,
                                  freeusd::gf::Quatd,
                                  freeusd::gf::Quatf,
                                  freeusd::gf::Matrix4d,
                                  freeusd::gf::Vec3d,
                                  freeusd::gf::Vec3f>;

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
  /// Ordered token list (``token[]`` / ``TfTokenVector``-shaped); used for ``xformOpOrder`` and similar.
  static Value MakeTokenArray(std::vector<freeusd::tf::Token> v);
  static Value MakeQuatd(freeusd::gf::Quatd v);
  static Value MakeQuatf(freeusd::gf::Quatf v);
  static Value MakeMatrix4d(freeusd::gf::Matrix4d v);
  static Value MakeVec3d(freeusd::gf::Vec3d v);
  static Value MakeVec3f(freeusd::gf::Vec3f v);

  bool IsEmpty() const noexcept;
  bool HoldsBool() const noexcept;
  bool HoldsInt32() const noexcept;
  bool HoldsInt64() const noexcept;
  bool HoldsFloat() const noexcept;
  bool HoldsDouble() const noexcept;
  bool HoldsString() const noexcept;
  bool HoldsToken() const noexcept;
  bool HoldsTokenArray() const noexcept;
  bool HoldsQuatd() const noexcept;
  bool HoldsQuatf() const noexcept;
  bool HoldsMatrix4d() const noexcept;
  bool HoldsVec3d() const noexcept;
  bool HoldsVec3f() const noexcept;

  bool GetBool(bool* out) const noexcept;
  bool GetInt32(std::int32_t* out) const noexcept;
  bool GetInt64(std::int64_t* out) const noexcept;
  bool GetFloat(float* out) const noexcept;
  bool GetDouble(double* out) const noexcept;
  bool GetString(std::string* out) const;
  bool GetToken(freeusd::tf::Token* out) const;
  bool GetTokenArray(std::vector<freeusd::tf::Token>* out) const;
  bool GetQuatd(freeusd::gf::Quatd* out) const noexcept;
  bool GetQuatf(freeusd::gf::Quatf* out) const noexcept;
  bool GetMatrix4d(freeusd::gf::Matrix4d* out) const noexcept;
  bool GetVec3d(freeusd::gf::Vec3d* out) const noexcept;
  bool GetVec3f(freeusd::gf::Vec3f* out) const noexcept;

  const ValuePayload& GetPayload() const noexcept { return payload_; }
  ValuePayload& GetPayload() noexcept { return payload_; }

  FREEUSD_API friend std::ostream& operator<<(std::ostream& os, const Value& v);

 private:
  explicit Value(ValuePayload p) : payload_(std::move(p)) {}

  ValuePayload payload_{};
};

}  // namespace freeusd::vt
