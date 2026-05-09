#include <cassert>
#include <string>

#include "freeusd/sdf/assetPath.hpp"

int main() {
  freeusd::sdf::AssetPath a;
  assert(a.IsEmpty());

  freeusd::sdf::AssetPath b{"./scene.usda"};
  assert(!b.IsEmpty());
  assert(b.GetPath() == std::string("./scene.usda"));

  b.SetPath("other.usdc");
  assert(b.GetPath() == "other.usdc");

  return 0;
}
