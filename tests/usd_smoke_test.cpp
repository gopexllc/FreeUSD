#include <cassert>
#include <filesystem>
#include <fstream>
#include <vector>

#include "freeusd/plug/registry.hpp"
#include "freeusd/sdf/layer.hpp"
#include "freeusd/sdf/path.hpp"
#include "freeusd/tf/token.hpp"
#include "freeusd/trace/collector.hpp"
#include "freeusd/usd/crateFile.hpp"
#include "freeusd/usd/stage.hpp"
#include "freeusd/usdGeom/xformable.hpp"
#include "freeusd/usdShade/tokens.hpp"
#include "freeusd/usdUtils/pipeline.hpp"
#include "freeusd/vt/value.hpp"

int main() {
  using freeusd::sdf::Layer;
  using freeusd::sdf::Path;
  using freeusd::tf::Token;
  using freeusd::usd::Stage;
  using freeusd::vt::Value;

  auto layer = Layer::NewAnonymous("anon");
  const Path world = Path::FromString("/World");
  const Path cube = Path::FromString("/World/Cube");
  layer->SetField(cube, Token("size"), Value::MakeDouble(2.5));
  layer->SetDefaultPrim("World");

  auto stage = Stage::AttachRootLayer(layer);
  assert(stage);
  const auto kids = stage->GetChildren(Path::AbsoluteRootPath());
  assert(!kids.empty());

  const auto prim = stage->GetPrimAtPath(cube);
  assert(prim.IsValid());
  assert(prim.GetName() == "Cube");
  assert(prim.GetParent().IsValid());
  assert(prim.GetParent().GetPath() == world);
  assert(prim.GetStage().get() == stage.get());
  const auto defp = stage->GetDefaultPrim();
  assert(defp.IsValid());
  assert(defp.GetPath() == world);
  assert(defp.GetName() == "World");
  const auto world_prim = stage->GetPrimAtPath(world);
  const auto under_world = world_prim.GetChildren();
  assert(!under_world.empty());

  assert(prim.HasAttribute(Token("size")));
  double d = 0;
  assert(prim.GetAttribute(Token("size")).GetDouble(&d));
  assert(d == 2.5);

  layer->SetTimeSample(cube, Token("height"), 1.0, Value::MakeDouble(10.0));
  layer->SetTimeSample(cube, Token("height"), 3.0, Value::MakeDouble(30.0));
  layer->SetField(cube, Token("height"), Value::MakeDouble(0.0));
  assert(prim.GetAttribute(Token("height"), 2.0).GetDouble(&d));
  assert(d == 10.0);
  assert(prim.GetAttribute(Token("height"), 3.0).GetDouble(&d));
  assert(d == 30.0);

  const std::vector<std::string> attrs = prim.ListAttributeNames();
  assert(attrs.size() >= 2u);
  const std::vector<double> ht_times = prim.ListAttributeSampleTimes(Token("height"));
  assert(ht_times.size() == 2u);

  std::vector<std::string> visited;
  stage->TraversePreorder([&](const freeusd::usd::Prim& p) {
    visited.push_back(p.GetPath().GetString());
    return true;
  });
  bool seen_world = false;
  bool seen_cube = false;
  for (const std::string& s : visited) {
    if (s == world.GetString()) {
      seen_world = true;
    }
    if (s == cube.GetString()) {
      seen_cube = true;
    }
  }
  assert(seen_world && seen_cube);
  bool cube_after_prune = false;
  stage->TraversePreorder([&](const freeusd::usd::Prim& p) {
    if (p.GetPath() == cube) {
      cube_after_prune = true;
    }
    return p.GetPath() != world;
  });
  assert(!cube_after_prune);

  assert(freeusd::usdShade::tokens::Material().GetText() == "Material");

  namespace fs = std::filesystem;
  {
    const fs::path p = fs::temp_directory_path() / "freeusd_smoke_ascii.usda";
    {
      std::ofstream o(p);
      o << "#usda 1.0\n(\n)\ndef X \"x\" {}\n";
    }
    std::string detail;
    assert(freeusd::usd::crate::DetectUsdFileKindFromPath(p.string(), &detail) ==
           freeusd::usd::crate::UsdFileKind::UsdaAscii);
    fs::remove(p);
  }
  {
    const fs::path p = fs::temp_directory_path() / "freeusd_smoke_crate.usdc";
    {
      std::ofstream o(p, std::ios::binary);
      const std::string m = std::string{freeusd::usd::crate::UsdcCrateIdentifier()};
      o.write(m.data(), static_cast<std::streamsize>(m.size()));
      o.put('\0');
    }
    assert(freeusd::usd::crate::DetectUsdFileKindFromPath(p.string(), nullptr) ==
           freeusd::usd::crate::UsdFileKind::UsdcCrate);
    fs::remove(p);
  }

  freeusd::plug::Registry::Get().RegisterPluginPaths({"/tmp/freeusd_nonexistent_plugin_path"});
  assert(!freeusd::plug::Registry::Get().RegisteredPluginPaths().empty());

  freeusd::trace::Collector::Get().Reset();
  freeusd::trace::Collector::Get().Push("scope");
  assert(freeusd::trace::Collector::Get().StackDepth() == 1u);
  freeusd::trace::Collector::Get().Pop();
  assert(freeusd::trace::Collector::Get().StackDepth() == 0u);

  freeusd::usdUtils::FlattenOptions fo;
  assert(fo.merge_authored_layer_metadata);
  fo.merge_authored_layer_metadata = false;
  assert(!fo.merge_authored_layer_metadata);

  const freeusd::usdGeom::Xformable xf(prim);
  assert(static_cast<bool>(xf));
  assert(xf.ComputeLocalToWorldTransform() == freeusd::gf::Matrix4d::Identity());

  return 0;
}
