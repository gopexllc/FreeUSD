#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "freeusd/export.hpp"
#include "freeusd/path.hpp"
#include "freeusd/value.hpp"

namespace freeusd {

enum class PrimSpecifier {
  Def,
  Over,
  Class,
};

struct AttributeSpec {
  Path path;
  std::string name;
  std::string type_name;
  std::string variability;
  Value default_value;
  bool custom{false};
};

struct PrimSpec {
  Path path;
  std::string name;
  std::string type_name;
  PrimSpecifier specifier{PrimSpecifier::Def};
  std::vector<AttributeSpec> attributes;
  std::vector<PrimSpec> children;
};

class FREEUSD_API Layer {
 public:
  explicit Layer(std::string identifier = {});

  const std::string& identifier() const noexcept { return identifier_; }
  void set_identifier(std::string identifier) { identifier_ = std::move(identifier); }

  const std::vector<PrimSpec>& root_prims() const noexcept { return root_prims_; }
  std::vector<PrimSpec>& root_prims() noexcept { return root_prims_; }

  void add_root_prim(PrimSpec prim);
  const PrimSpec* find_prim(std::string_view path) const;

 private:
  std::string identifier_;
  std::vector<PrimSpec> root_prims_;
};

FREEUSD_API const char* to_string(PrimSpecifier specifier);

}  // namespace freeusd
