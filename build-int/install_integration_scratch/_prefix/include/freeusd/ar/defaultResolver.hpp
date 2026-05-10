#pragma once

#include <string>
#include <string_view>

#include "freeusd/ar/resolver.hpp"
#include "freeusd/export.hpp"

namespace freeusd::ar {

/// Minimal filesystem-oriented resolver (no full ArDefaultResolver parity).
class FREEUSD_API DefaultResolver final : public Resolver {
 public:
  explicit DefaultResolver(std::string anchor_directory = {});

  std::string Resolve(std::string_view asset_path) override;

 private:
  std::string anchor_;
};

}  // namespace freeusd::ar
