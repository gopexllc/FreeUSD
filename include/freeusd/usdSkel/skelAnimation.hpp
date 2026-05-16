#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include "freeusd/export.hpp"
#include "freeusd/gf/quatf.hpp"
#include "freeusd/gf/vec3f.hpp"
#include "freeusd/sdf/path.hpp"
#include "freeusd/usd/prim.hpp"

namespace freeusd::usd {
class Stage;
}

namespace freeusd::usdSkel {

/// Per-joint TRS sample at a time (glTF animation channel shaped).
struct FREEUSD_API JointTransform {
  freeusd::gf::Vec3f translation{};
  freeusd::gf::Quatf rotation = freeusd::gf::Quatf::Identity();
  freeusd::gf::Vec3f scale{1.0f, 1.0f, 1.0f};
  bool has_translation{false};
  bool has_rotation{false};
  bool has_scale{false};
};

/// ``UsdSkelAnimation``-shaped helper: sampled ``translations`` / ``rotations`` / ``scales`` (glTF TRS channels).
struct FREEUSD_API SkelAnimation {
  freeusd::usd::Prim prim;

  SkelAnimation() = default;
  explicit SkelAnimation(freeusd::usd::Prim p) : prim(std::move(p)) {}

  explicit operator bool() const noexcept { return prim.IsValid(); }
  const freeusd::usd::Prim& GetPrim() const noexcept { return prim; }

  std::vector<std::string> GetJointNames() const;

  bool GetTranslations(std::vector<freeusd::gf::Vec3f>* out, double time = 1.0) const;
  bool GetRotations(std::vector<freeusd::gf::Quatf>* out, double time = 1.0) const;
  bool GetScales(std::vector<freeusd::gf::Vec3f>* out, double time = 1.0) const;

  std::vector<double> ListTranslationSampleTimes() const;
  std::vector<double> ListRotationSampleTimes() const;
  std::vector<double> ListScaleSampleTimes() const;

  /// Sample one joint's TRS channels at \p time (false when anim/joint index is invalid).
  static bool SampleJointTransform(const std::shared_ptr<const freeusd::usd::Stage>& stage,
                                   const freeusd::sdf::Path& anim_path, std::size_t joint_index, double time,
                                   JointTransform* out);
};

}  // namespace freeusd::usdSkel
