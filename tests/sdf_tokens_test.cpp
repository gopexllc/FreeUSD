#include <cassert>
#include <string>

#include "freeusd/sdf/tokens.hpp"

int main() {
  using freeusd::sdf::tokens::SubLayers;
  assert(SubLayers().GetText() == std::string("subLayers"));
  assert(freeusd::sdf::tokens::Kind().GetText() == std::string("kind"));
  return 0;
}
