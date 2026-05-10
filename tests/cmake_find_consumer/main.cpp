#include <freeusd/sdf/layer.hpp>
#include <freeusd/usd/stage.hpp>
int main() {
  auto s = freeusd::usd::Stage::AttachRootLayer(freeusd::sdf::Layer::NewAnonymous());
  return s ? 0 : 1;
}
