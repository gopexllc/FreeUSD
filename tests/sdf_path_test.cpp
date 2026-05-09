#include <cassert>
#include <string>

#include "freeusd/sdf/path.hpp"

int main() {
  using freeusd::sdf::Path;
  using freeusd::sdf::PathGetPrefixes;

  const Path root = Path::AbsoluteRootPath();
  assert(root.IsAbsoluteRootPath());
  assert(root.GetParentPath().IsEmpty());

  const Path world = Path::FromString("/World");
  assert(world.IsPrimPath());
  assert(world.GetParentPath() == root);

  const Path cube = Path::FromString("/World/Cube");
  assert(cube.IsPrimPath());
  assert(cube.GetPrimPath() == cube);
  assert(cube.GetParentPath() == world);

  const Path prop = Path::FromString("/World/Cube.size");
  assert(prop.IsPropertyPath());
  assert(prop.GetPrimPath() == cube);
  assert(prop.GetParentPath() == cube);
  assert(prop.GetName() == "size");

  const Path bad = Path::FromString("/World//Cube");
  assert(bad.IsEmpty() || bad.GetText() == "/World/Cube");  // normalization yields valid

  const Path bad2 = Path::FromString("relative/path");
  assert(bad2.IsEmpty());

  const auto prefs = PathGetPrefixes(prop);
  assert(!prefs.empty());
  assert(prefs.front() == root);

  return 0;
}
