#pragma once

#include <map>
#include <optional>
#include <string>
#include <vector>

#include "freeusd/export.hpp"
#include "freeusd/vt/value.hpp"

namespace freeusd::sdf {

/// Default value plus optional time samples (Sdf-shaped, clean-room).
struct FREEUSD_API FieldOpinion {
  std::optional<freeusd::vt::Value> default_value;
  std::map<double, freeusd::vt::Value> time_samples;

  bool HasAny() const noexcept { return default_value.has_value() || !time_samples.empty(); }

  void SetDefault(freeusd::vt::Value v) { default_value = std::move(v); }
  void ClearDefault() { default_value.reset(); }

  void SetSample(double time, freeusd::vt::Value v);
  void ClearSamples() noexcept { time_samples.clear(); }

  /// Resolves at \p time: before the first sample, uses default when set else the earliest sample; after the last
  /// sample, holds the last; between two samples, **linear** interpolation for ``float`` / ``double`` / ``int32`` /
  /// ``int64`` / ``vec3d`` / ``vec3f`` / ``matrix4d`` (per-element), **slerp** for ``quatd`` / ``quatf``; non-interpolable types use the lower
  /// bracket sample. If there are no samples, returns the default when set.
  bool EvaluateAt(double time, freeusd::vt::Value* out) const;

  bool GetExactSample(double time, freeusd::vt::Value* out) const;

  std::vector<double> ListTimes() const;
};

}  // namespace freeusd::sdf
