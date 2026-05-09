#include <cassert>
#include <string>

#include "freeusd/sdf/tokens.hpp"

int main() {
  using freeusd::sdf::tokens::SubLayers;
  assert(SubLayers().GetText() == std::string("subLayers"));
  assert(freeusd::sdf::tokens::SubLayerOffsets().GetText() == std::string("subLayerOffsets"));
  assert(freeusd::sdf::tokens::Comment().GetText() == std::string("comment"));
  assert(freeusd::sdf::tokens::CustomLayerData().GetText() == std::string("customLayerData"));
  assert(freeusd::sdf::tokens::Kind().GetText() == std::string("kind"));
  return 0;
}
