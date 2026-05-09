#include <cassert>
#include <memory>

#include "freeusd/sdf/layer.hpp"
#include "freeusd/usd/editTarget.hpp"

int main() {
  freeusd::usd::EditTarget empty{};
  assert(!empty.IsValid());

  auto layer = freeusd::sdf::Layer::NewAnonymous("t");
  freeusd::usd::EditTarget t;
  t.layer = layer;
  assert(t.IsValid());
  assert(t.layer == layer);

  return 0;
}
