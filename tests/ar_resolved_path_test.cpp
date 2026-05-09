#include <cassert>
#include <string>

#include "freeusd/ar/resolvedPath.hpp"

int main() {
  freeusd::ar::ResolvedPath a;
  assert(a.IsEmpty());

  freeusd::ar::ResolvedPath b{std::string{"/tmp/x.usda"}};
  assert(!b.IsEmpty());
  assert(b.GetPath() == "/tmp/x.usda");

  b.SetPath("y.usdc");
  assert(b.GetPath() == "y.usdc");

  return 0;
}
