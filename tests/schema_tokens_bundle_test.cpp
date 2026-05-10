#include <cassert>
#include <string>

#include "freeusd/usd/kindTokens.hpp"
#include "freeusd/usd/schemaTokens.hpp"

int main() {
  assert(freeusd::usd::kindTokens::Component().GetText() == std::string("component"));
  assert(freeusd::usdGeom::tokens::Mesh().GetText() == std::string("Mesh"));
  assert(freeusd::usdGeom::tokens::Camera().GetText() == std::string("Camera"));
  assert(freeusd::usdGeom::tokens::points().GetText() == std::string("points"));
  assert(freeusd::usdShade::tokens::Material().GetText() == std::string("Material"));
  assert(freeusd::usdPhysics::tokens::PhysicsScene().GetText() == std::string("PhysicsScene"));
  assert(freeusd::usdRender::tokens::RenderSettings().GetText() == std::string("RenderSettings"));
  assert(freeusd::usdUI::tokens::ui_displayName().GetText() == std::string("ui:displayName"));
  assert(freeusd::usd::schemaDataTokens::ModelAPI().GetText() == std::string("ModelAPI"));
  return 0;
}
