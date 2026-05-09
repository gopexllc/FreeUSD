#include <cassert>
#include <filesystem>
#include <fstream>
#include <random>
#include <string>

#include "freeusd/sdf/path.hpp"
#include "freeusd/usd/stage.hpp"

int main() {
  namespace fs = std::filesystem;
  using freeusd::sdf::Path;
  using freeusd::usd::RootLayerSublayersPolicy;
  using freeusd::usd::Stage;

  std::mt19937_64 rng{std::random_device{}()};
  const fs::path base = fs::temp_directory_path() / ("freeusd_stage_open_" + std::to_string(rng()));
  fs::create_directories(base);

  const fs::path sub_path = base / "sub.usda";
  const fs::path root_path = base / "root.usda";
  {
    std::ofstream sub(sub_path);
    sub << "#usda 1.0\n(\n)\ndef Xform \"FromSub\"\n(\n)\n{\n}\n";
    std::ofstream root(root_path);
    root << "#usda 1.0\n(\n    subLayers = [@./sub.usda@]\n)\ndef Xform \"Root\"\n(\n)\n{\n}\n";
  }

  std::string err;
  auto none = Stage::OpenFromRootFile(root_path.string(), RootLayerSublayersPolicy::None, &err);
  assert(none);
  assert(!none->PrimPathInUse(Path::FromString("/FromSub")));

  auto deep = Stage::OpenFromRootFile(root_path.string(), RootLayerSublayersPolicy::DepthFirst, &err);
  assert(deep);
  assert(deep->PrimPathInUse(Path::FromString("/FromSub")));
  assert(deep->GetComposeLayers().size() >= 2u);

  auto shallow = Stage::OpenFromRootFile(root_path.string(), RootLayerSublayersPolicy::Shallow, &err);
  assert(shallow);
  assert(shallow->PrimPathInUse(Path::FromString("/FromSub")));

  fs::remove_all(base);
  return 0;
}
