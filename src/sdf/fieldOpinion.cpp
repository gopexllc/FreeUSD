#include "freeusd/sdf/fieldOpinion.hpp"

#include <algorithm>
#include <cmath>

#include "freeusd/gf/matrix4d.hpp"
#include "freeusd/gf/quatd.hpp"
#include "freeusd/gf/quatf.hpp"
#include "freeusd/gf/vec3d.hpp"
#include "freeusd/gf/vec3f.hpp"

namespace freeusd::sdf {
namespace {

double clamp_unit_interval(double x) { return std::clamp(x, 0.0, 1.0); }

freeusd::gf::Quatd slerp_quatd(const freeusd::gf::Quatd& a, const freeusd::gf::Quatd& b, double t) {
  t = clamp_unit_interval(t);
  double cos_half = a.real * b.real + a.i * b.i + a.j * b.j + a.k * b.k;
  freeusd::gf::Quatd b2 = b;
  if (cos_half < 0.0) {
    cos_half = -cos_half;
    b2 = freeusd::gf::Quatd{-b.real, -b.i, -b.j, -b.k};
  }
  if (cos_half > 1.0 - 1e-9 || cos_half < -1.0 + 1e-9) {
    const double u = 1.0 - t;
    return freeusd::gf::Quatd{a.real * u + b2.real * t, a.i * u + b2.i * t, a.j * u + b2.j * t, a.k * u + b2.k * t}
        .Normalized();
  }
  cos_half = std::clamp(cos_half, -1.0, 1.0);
  const double half_theta = std::acos(cos_half);
  const double sin_half = std::sin(half_theta);
  if (!(sin_half > 1e-12)) {
    return a.Normalized();
  }
  const double inv = 1.0 / sin_half;
  const double k0 = std::sin((1.0 - t) * half_theta) * inv;
  const double k1 = std::sin(t * half_theta) * inv;
  return freeusd::gf::Quatd{a.real * k0 + b2.real * k1, a.i * k0 + b2.i * k1, a.j * k0 + b2.j * k1, a.k * k0 + b2.k * k1}
      .Normalized();
}

// Returns true if `out` was filled by interpolation; false means use the lower-bracket value `a`.
bool try_interpolate_samples(const freeusd::vt::Value& a, const freeusd::vt::Value& b, double alpha, freeusd::vt::Value* out) {
  if (!out) {
    return false;
  }
  alpha = clamp_unit_interval(alpha);
  if (a.GetPayload().index() != b.GetPayload().index()) {
    return false;
  }
  if (a.HoldsDouble()) {
    double x0 = 0;
    double x1 = 0;
    if (!a.GetDouble(&x0) || !b.GetDouble(&x1)) {
      return false;
    }
    *out = freeusd::vt::Value::MakeDouble(x0 + alpha * (x1 - x0));
    return true;
  }
  if (a.HoldsFloat()) {
    float x0 = 0;
    float x1 = 0;
    if (!a.GetFloat(&x0) || !b.GetFloat(&x1)) {
      return false;
    }
    *out = freeusd::vt::Value::MakeFloat(static_cast<float>(static_cast<double>(x0) + alpha * (static_cast<double>(x1) - x0)));
    return true;
  }
  if (a.HoldsInt32()) {
    std::int32_t x0 = 0;
    std::int32_t x1 = 0;
    if (!a.GetInt32(&x0) || !b.GetInt32(&x1)) {
      return false;
    }
    const double v = static_cast<double>(x0) + alpha * (static_cast<double>(x1) - static_cast<double>(x0));
    *out = freeusd::vt::Value::MakeInt32(static_cast<std::int32_t>(std::llround(v)));
    return true;
  }
  if (a.HoldsInt64()) {
    std::int64_t x0 = 0;
    std::int64_t x1 = 0;
    if (!a.GetInt64(&x0) || !b.GetInt64(&x1)) {
      return false;
    }
    const double v = static_cast<double>(x0) + alpha * (static_cast<double>(x1) - static_cast<double>(x0));
    *out = freeusd::vt::Value::MakeInt64(static_cast<std::int64_t>(std::llround(v)));
    return true;
  }
  if (a.HoldsVec3d()) {
    freeusd::gf::Vec3d x0{};
    freeusd::gf::Vec3d x1{};
    if (!a.GetVec3d(&x0) || !b.GetVec3d(&x1)) {
      return false;
    }
    const double u = 1.0 - alpha;
    freeusd::gf::Vec3d v{};
    v.set(x0.x() * u + x1.x() * alpha, x0.y() * u + x1.y() * alpha, x0.z() * u + x1.z() * alpha);
    *out = freeusd::vt::Value::MakeVec3d(v);
    return true;
  }
  if (a.HoldsVec3f()) {
    freeusd::gf::Vec3f x0{};
    freeusd::gf::Vec3f x1{};
    if (!a.GetVec3f(&x0) || !b.GetVec3f(&x1)) {
      return false;
    }
    const float u = static_cast<float>(1.0 - alpha);
    const float t = static_cast<float>(alpha);
    freeusd::gf::Vec3f v{};
    v.set(x0.x() * u + x1.x() * t, x0.y() * u + x1.y() * t, x0.z() * u + x1.z() * t);
    *out = freeusd::vt::Value::MakeVec3f(v);
    return true;
  }
  if (a.HoldsMatrix4d()) {
    freeusd::gf::Matrix4d x0{};
    freeusd::gf::Matrix4d x1{};
    if (!a.GetMatrix4d(&x0) || !b.GetMatrix4d(&x1)) {
      return false;
    }
    freeusd::gf::Matrix4d r{};
    const double u = 1.0 - alpha;
    for (std::size_t i = 0; i < 16; ++i) {
      r.m[i] = x0.m[i] * u + x1.m[i] * alpha;
    }
    *out = freeusd::vt::Value::MakeMatrix4d(r);
    return true;
  }
  if (a.HoldsQuatd()) {
    freeusd::gf::Quatd x0{};
    freeusd::gf::Quatd x1{};
    if (!a.GetQuatd(&x0) || !b.GetQuatd(&x1)) {
      return false;
    }
    *out = freeusd::vt::Value::MakeQuatd(slerp_quatd(x0, x1, alpha));
    return true;
  }
  if (a.HoldsQuatf()) {
    freeusd::gf::Quatf x0{};
    freeusd::gf::Quatf x1{};
    if (!a.GetQuatf(&x0) || !b.GetQuatf(&x1)) {
      return false;
    }
    const freeusd::gf::Quatd d0{static_cast<double>(x0.real), static_cast<double>(x0.i), static_cast<double>(x0.j),
                               static_cast<double>(x0.k)};
    const freeusd::gf::Quatd d1{static_cast<double>(x1.real), static_cast<double>(x1.i), static_cast<double>(x1.j),
                               static_cast<double>(x1.k)};
    const freeusd::gf::Quatd dr = slerp_quatd(d0, d1, alpha);
    *out = freeusd::vt::Value::MakeQuatf(freeusd::gf::Quatf{static_cast<float>(dr.real), static_cast<float>(dr.i),
                                                          static_cast<float>(dr.j), static_cast<float>(dr.k)});
    return true;
  }
  // bool, string, token, token[]: bracketing uses lower sample (OpenUSD-style non-interpolable).
  return false;
}

}  // namespace

void FieldOpinion::SetSample(double time, freeusd::vt::Value v) { time_samples[time] = std::move(v); }

bool FieldOpinion::EvaluateAt(double time, freeusd::vt::Value* out) const {
  if (!out) {
    return false;
  }
  if (!time_samples.empty()) {
    auto hi = time_samples.upper_bound(time);
    if (hi == time_samples.begin()) {
      // Every sample is strictly after `time`.
      if (default_value) {
        *out = *default_value;
        return true;
      }
      *out = time_samples.begin()->second;
      return true;
    }
    const auto lo = std::prev(hi);
    if (hi != time_samples.end() && time > lo->first) {
      const double t0 = lo->first;
      const double t1 = hi->first;
      const double alpha = (time - t0) / (t1 - t0);
      if (try_interpolate_samples(lo->second, hi->second, alpha, out)) {
        return true;
      }
    }
    *out = lo->second;
    return true;
  }
  if (default_value) {
    *out = *default_value;
    return true;
  }
  return false;
}

bool FieldOpinion::GetExactSample(double time, freeusd::vt::Value* out) const {
  if (!out) {
    return false;
  }
  const auto it = time_samples.find(time);
  if (it == time_samples.end()) {
    return false;
  }
  *out = it->second;
  return true;
}

std::vector<double> FieldOpinion::ListTimes() const {
  std::vector<double> out;
  out.reserve(time_samples.size());
  for (const auto& e : time_samples) {
    out.push_back(e.first);
  }
  return out;
}

}  // namespace freeusd::sdf
