#pragma once

#include <memory>

#include "freeusd/export.hpp"
#include "freeusd/sdf/path.hpp"
#include "freeusd/tf/token.hpp"
#include "freeusd/vt/value.hpp"

namespace freeusd::usd {

class Stage;

/// Handle to a prim on a stage (UsdPrim-shaped surface, clean-room).
class FREEUSD_API Prim {
 public:
  Prim() = default;
  Prim(std::weak_ptr<const Stage> stage, freeusd::sdf::Path path);

  bool IsValid() const noexcept;
  freeusd::sdf::Path GetPath() const noexcept { return path_; }

  bool HasAttribute(const freeusd::tf::Token& name) const;
  freeusd::vt::Value GetAttribute(const freeusd::tf::Token& name) const;

 private:
  std::shared_ptr<const Stage> lock_stage() const;
  std::weak_ptr<const Stage> stage_;
  freeusd::sdf::Path path_{};
};

}  // namespace freeusd::usd
