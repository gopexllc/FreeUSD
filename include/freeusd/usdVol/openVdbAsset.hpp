#pragma once

#include "freeusd/export.hpp"
#include "freeusd/sdf/path.hpp"
#include "freeusd/usd/prim.hpp"

namespace freeusd::usd {
class Stage;
}

namespace freeusd::usdVol {

/// ``OpenVDBAsset``-shaped helper: read common volume field asset inputs at a time code.
struct FREEUSD_API OpenVDBAsset {
  freeusd::usd::Prim prim;

  OpenVDBAsset() = default;
  explicit OpenVDBAsset(freeusd::usd::Prim p) : prim(std::move(p)) {}

  static OpenVDBAsset ReadFromPrim(const std::shared_ptr<const freeusd::usd::Stage>& stage,
                                   const freeusd::sdf::Path& path);

  explicit operator bool() const noexcept { return prim.IsValid(); }

  bool IsOpenVDBAsset() const;

  bool GetFilePath(std::string* out_path, double time = 1.0) const;
  bool GetFieldName(std::string* out_name, double time = 1.0) const;
};

}  // namespace freeusd::usdVol
