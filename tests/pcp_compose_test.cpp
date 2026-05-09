#include <cassert>
#include <memory>

#include "freeusd/pcp/compose.hpp"
#include "freeusd/sdf/layer.hpp"

int main() {
  using freeusd::pcp::ComposeSublayers;
  using freeusd::sdf::Layer;

  auto root = Layer::NewAnonymous("root.usda");
  auto sub_a = Layer::NewAnonymous("./a.usda");
  auto sub_b = Layer::NewAnonymous("./b.usda");

  root->SetSubLayers({"./a.usda", "./nosuch.usda", "./b.usda"});

  auto stack = ComposeSublayers(root, [=](const std::string& path) -> std::shared_ptr<Layer> {
    if (path == "./a.usda") {
      return sub_a;
    }
    if (path == "./b.usda") {
      return sub_b;
    }
    return {};
  });

  assert(!stack.IsEmpty());
  const auto& layers = stack.GetLayers();
  assert(layers.size() == 3);
  assert(layers[0] == root);
  assert(layers[1] == sub_a);
  assert(layers[2] == sub_b);

  stack.Clear();
  assert(stack.IsEmpty());

  auto empty_stack = ComposeSublayers({}, [](const std::string&) {
    return Layer::NewAnonymous({});
  });
  assert(empty_stack.IsEmpty());

  {
    using freeusd::pcp::ComposeSublayersDepthFirst;
    auto r2 = Layer::NewAnonymous("root2");
    auto sx = Layer::NewAnonymous("subx");
    auto sy = Layer::NewAnonymous("suby");
    r2->SetSubLayers({"subx.usda"});
    sx->SetSubLayers({"suby.usda"});

    auto dstack = ComposeSublayersDepthFirst(r2, [=](const std::string& path) -> std::shared_ptr<Layer> {
      if (path == "subx.usda") {
        return sx;
      }
      if (path == "suby.usda") {
        return sy;
      }
      return {};
    });
    assert(dstack.GetLayers().size() == 3);
    assert(dstack.GetLayers()[0] == r2);
    assert(dstack.GetLayers()[1] == sx);
    assert(dstack.GetLayers()[2] == sy);
  }

  return 0;
}
