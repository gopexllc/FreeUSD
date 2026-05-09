#include <cassert>
#include <string>

#include "freeusd/usd/tokens.hpp"

int main() {
  assert(freeusd::usd::tokens::References().GetText() == std::string("references"));
  assert(freeusd::usd::tokens::Relocates().GetText() == std::string("relocates"));
  return 0;
}
