#include <cassert>

#include "freeusd/gf/quatd.hpp"

int main() {
  using freeusd::gf::Quatd;
  const Quatd id = Quatd::Identity();
  assert(id.real == 1.0 && id.i == 0.0 && id.j == 0.0 && id.k == 0.0);

  Quatd q{0.0, 1.0, 0.0, 0.0};
  assert((q == Quatd{0.0, 1.0, 0.0, 0.0}));

  return 0;
}
