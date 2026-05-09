#include <cassert>
#include <sstream>
#include <string>

#include "freeusd/usd/timeCode.hpp"

int main() {
  using freeusd::usd::TimeCode;

  const TimeCode d = TimeCode::Default();
  const TimeCode e = TimeCode::EarliestTime();
  const TimeCode n = TimeCode::FromDouble(3.5);

  assert(d.IsDefault() && !d.IsNumeric());
  assert(e.IsEarliestTime());
  assert(n.IsNumeric() && n.GetValue() == 3.5);
  assert(d == TimeCode::Default());
  assert(n == TimeCode::FromDouble(3.5));
  assert(d != n);

  std::ostringstream oss;
  oss << n;
  assert(oss.str().find('3') != std::string::npos);

  return 0;
}
