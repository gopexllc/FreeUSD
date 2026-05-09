#include <cassert>

#include "freeusd/gf/range1d.hpp"

int main() {
  using freeusd::gf::Range1d;

  Range1d empty{};
  assert(empty.IsEmpty());

  Range1d u = Range1d::UnitInterval();
  assert(!u.IsEmpty());
  assert(u.min == 0.0 && u.max == 1.0);

  Range1d r{2.0, 5.0};
  assert(!r.IsEmpty());
  assert((r == Range1d{2.0, 5.0}));

  return 0;
}
