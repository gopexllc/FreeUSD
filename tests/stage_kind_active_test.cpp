#include <cassert>
#include <string>

#include "freeusd/pcp/layerStack.hpp"
#include "freeusd/sdf/layer.hpp"
#include "freeusd/sdf/path.hpp"
#include "freeusd/tf/token.hpp"
#include "freeusd/usd/stage.hpp"

#ifndef FREEUSD_TEST_FIXTURES_DIR
#error "FREEUSD_TEST_FIXTURES_DIR must be set by CMake for stage_kind_active_test"
#endif

namespace {

std::string fixture(const char* name) {
  return std::string(FREEUSD_TEST_FIXTURES_DIR) + "/" + name;
}

}  // namespace

int main() {
  using freeusd::pcp::LayerStack;
  using freeusd::sdf::Layer;
  using freeusd::sdf::Path;
  using freeusd::tf::Token;
  using freeusd::usd::Stage;

  auto strong = Layer::NewAnonymous("strong");
  auto weak = Layer::NewAnonymous("weak");
  const Path prim = Path::FromString("/Root/Prim");

  weak->SetPrimKind(prim, Token("Mesh"));
  strong->SetPrimKind(prim, Token("Xform"));
  strong->SetPrimActive(prim, false);

  LayerStack stack;
  stack.Append(strong);
  stack.Append(weak);
  auto stage = Stage::AttachLayerStack(stack);
  assert(stage);

  const auto p = stage->GetPrimAtPath(prim);
  assert(p.IsValid());
  assert(p.HasPrimKind());
  assert(p.GetPrimKind().GetText() == std::string("Xform"));
  assert(p.HasPrimActiveOpinion());
  assert(!p.IsActive());

  {
    std::string err;
    auto refs_stage = Stage::OpenFromRootFile(fixture("parity_kind_active_refs.usda"),
                                              freeusd::usd::RootLayerSublayersPolicy::DepthFirst, &err);
    assert(refs_stage && err.empty());
    const Path ref_host = Path::FromString("/World/RefHost");
    const Path payload_host = Path::FromString("/World/PayloadHost");
    const Path inherit_host = Path::FromString("/World/InheritHost");
    assert(refs_stage->ResolveHasPrimKind(ref_host));
    assert(refs_stage->ResolvePrimKind(ref_host).GetText() == std::string("component"));
    assert(refs_stage->ResolveHasPrimActiveOpinion(ref_host));
    assert(!refs_stage->ResolvePrimActive(ref_host));
    assert(refs_stage->ResolvePrimKind(payload_host).GetText() == std::string("group"));
    assert(refs_stage->ResolvePrimKind(inherit_host).GetText() == std::string("assembly"));
    assert(refs_stage->ResolvePrimActive(inherit_host));
    assert(!refs_stage->ResolveHasPrimActiveOpinion(inherit_host));
  }

  {
    std::string err;
    auto spec_stage = Stage::OpenFromRootFile(fixture("parity_kind_active_specializes.usda"),
                                              freeusd::usd::RootLayerSublayersPolicy::DepthFirst, &err);
    assert(spec_stage && err.empty());
    const Path spec_host = Path::FromString("/World/SpecHost");
    assert(spec_stage->ResolveHasPrimKind(spec_host));
    assert(spec_stage->ResolvePrimKind(spec_host).GetText() == std::string("assembly"));
    assert(spec_stage->ResolveHasPrimActiveOpinion(spec_host));
    assert(!spec_stage->ResolvePrimActive(spec_host));
  }

  return 0;
}
