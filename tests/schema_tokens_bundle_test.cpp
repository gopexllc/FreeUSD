#include <cassert>
#include <string>

#include "freeusd/usd/kindTokens.hpp"
#include "freeusd/usd/schemaTokens.hpp"

int main() {
  assert(freeusd::usd::kindTokens::Component().GetText() == std::string("component"));
  assert(freeusd::usdGeom::tokens::Mesh().GetText() == std::string("Mesh"));
  assert(freeusd::usdShade::tokens::Material().GetText() == std::string("Material"));
  assert(freeusd::usdPhysics::tokens::Scene().GetText() == std::string("PhysicsScene"));
  assert(freeusd::usdRender::tokens::Settings().GetText() == std::string("RenderSettings"));
  return 0;
}
