#include <cassert>
#include <sstream>
#include <string>

#include "freeusd/version.hpp"

int main() {
  const std::string v = freeusd::version_string();
  assert(!v.empty());

  std::uint32_t maj{}, min{}, pat{};
  freeusd::version_parts(maj, min, pat);
  std::ostringstream oss;
  oss << maj << '.' << min << '.' << pat;
  assert(oss.str() == v);

  return 0;
}
