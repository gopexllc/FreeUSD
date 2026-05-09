#include "freeusd/sdf/fieldOpinion.hpp"

namespace freeusd::sdf {

void FieldOpinion::SetSample(double time, freeusd::vt::Value v) { time_samples[time] = std::move(v); }

bool FieldOpinion::EvaluateAt(double time, freeusd::vt::Value* out) const {
  if (!out) {
    return false;
  }
  if (!time_samples.empty()) {
    auto it = time_samples.upper_bound(time);
    if (it == time_samples.begin()) {
      // Every sample is strictly after `time`.
      if (default_value) {
        *out = *default_value;
        return true;
      }
      *out = time_samples.begin()->second;
      return true;
    }
    --it;
    *out = it->second;
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
