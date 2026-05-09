#include <cassert>

#include "freeusd/sdf/layerOffset.hpp"

int main() {
  freeusd::sdf::LayerOffset id{};
  assert(id.IsIdentity());

  freeusd::sdf::LayerOffset o{};
  o.offset = 2.0;
  assert(!o.IsIdentity());

  return 0;
}
