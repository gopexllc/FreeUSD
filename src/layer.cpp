#include "freeusd/layer.hpp"

namespace freeusd {
namespace {

const PrimSpec* find_prim_in(const std::vector<PrimSpec>& prims, std::string_view path) {
  for (const PrimSpec& prim : prims) {
    if (prim.path.string() == path) {
      return &prim;
    }
    if (const PrimSpec* child = find_prim_in(prim.children, path)) {
      return child;
    }
  }
  return nullptr;
}

}  // namespace

Layer::Layer(std::string identifier) : identifier_(std::move(identifier)) {}

void Layer::add_root_prim(PrimSpec prim) {
  root_prims_.push_back(std::move(prim));
}

const PrimSpec* Layer::find_prim(std::string_view path) const {
  return find_prim_in(root_prims_, path);
}

const char* to_string(PrimSpecifier specifier) {
  switch (specifier) {
    case PrimSpecifier::Def:
      return "def";
    case PrimSpecifier::Over:
      return "over";
    case PrimSpecifier::Class:
      return "class";
  }
  return "def";
}

}  // namespace freeusd
