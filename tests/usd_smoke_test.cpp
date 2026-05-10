#include <cassert>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <vector>

#include "freeusd/gf/matrix4d.hpp"
#include "freeusd/gf/vec3d.hpp"
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
  {
    const fs::path p = fs::temp_directory_path() / "freeusd_smoke_crate_boot.usdc";
    std::vector<unsigned char> buf(128, 0);
    const std::string m = std::string{freeusd::usd::crate::UsdcCrateIdentifier()};
    std::memcpy(buf.data(), m.data(), m.size());
    buf[8] = 0;
    buf[9] = 8;
    buf[10] = 0;
    const std::int64_t toc = 88;
    std::uint64_t u = 0;
    std::memcpy(&u, &toc, sizeof(u));
    for (int i = 0; i < 8; ++i) {
      buf[16 + static_cast<std::size_t>(i)] = static_cast<unsigned char>((u >> (8 * i)) & 0xffu);
    }
    {
      std::ofstream o(p, std::ios::binary);
      o.write(reinterpret_cast<const char*>(buf.data()), static_cast<std::streamsize>(buf.size()));
    }
    freeusd::usd::crate::UsdcCrateBootstrap boot{};
    std::string err;
    assert(freeusd::usd::crate::ReadUsdCrateBootstrapFromPath(p.string(), boot, &err));
    assert(err.empty());
    assert(boot.file_version_major == 0);
    assert(boot.file_version_minor == 8);
    assert(boot.file_version_patch == 0);
    assert(boot.toc_byte_offset == 88);
    std::string bad_err;
    assert(!freeusd::usd::crate::ReadUsdCrateBootstrapFromPath(p.string() + ".missing", boot, &bad_err));
    assert(!bad_err.empty());
    fs::remove(p);
  }
  {
    const fs::path p = fs::temp_directory_path() / "freeusd_smoke_crate_toc.usdc";
    std::vector<unsigned char> buf(160, 0);
    const std::string m = std::string{freeusd::usd::crate::UsdcCrateIdentifier()};
    std::memcpy(buf.data(), m.data(), m.size());
    buf[8] = 0;
    buf[9] = 8;
    buf[10] = 0;
    {
      const std::int64_t toc = 88;
      std::uint64_t u = 0;
      std::memcpy(&u, &toc, sizeof(u));
      for (int i = 0; i < 8; ++i) {
        buf[16 + static_cast<std::size_t>(i)] = static_cast<unsigned char>((u >> (8 * i)) & 0xffu);
      }
    }
    // TOC @88: uint64 count=2 LE, then two 32-byte section records.
    {
      std::uint64_t count = 2;
      for (int i = 0; i < 8; ++i) {
        buf[88 + static_cast<std::size_t>(i)] = static_cast<unsigned char>((count >> (8 * i)) & 0xffu);
      }
    }
    {
      unsigned char* rec = buf.data() + 96;
      const char nm[] = "TOKENS";
      std::memcpy(rec, nm, sizeof(nm));
      const std::int64_t st = 0;
      const std::int64_t sz = 0;
      std::uint64_t ust = 0;
      std::uint64_t usz = 0;
      std::memcpy(&ust, &st, sizeof(ust));
      std::memcpy(&usz, &sz, sizeof(usz));
      for (int i = 0; i < 8; ++i) {
        rec[16 + static_cast<std::size_t>(i)] = static_cast<unsigned char>((ust >> (8 * i)) & 0xffu);
        rec[24 + static_cast<std::size_t>(i)] = static_cast<unsigned char>((usz >> (8 * i)) & 0xffu);
      }
    }
    {
      unsigned char* rec = buf.data() + 128;
      const char nm[] = "PATHS";
      std::memcpy(rec, nm, sizeof(nm));
      const std::int64_t st = 120;
      const std::int64_t sz = 40;
      std::uint64_t ust = 0;
      std::uint64_t usz = 0;
      std::memcpy(&ust, &st, sizeof(ust));
      std::memcpy(&usz, &sz, sizeof(usz));
      for (int i = 0; i < 8; ++i) {
        rec[16 + static_cast<std::size_t>(i)] = static_cast<unsigned char>((ust >> (8 * i)) & 0xffu);
        rec[24 + static_cast<std::size_t>(i)] = static_cast<unsigned char>((usz >> (8 * i)) & 0xffu);
      }
    }
    {
      std::ofstream o(p, std::ios::binary);
      o.write(reinterpret_cast<const char*>(buf.data()), static_cast<std::streamsize>(buf.size()));
    }
    freeusd::usd::crate::UsdcCrateToc toc{};
    std::string err;
    assert(freeusd::usd::crate::ReadUsdCrateTocFromPath(p.string(), toc, 16, &err));
    assert(err.empty());
    assert(toc.section_count == 2u);
    assert(toc.sections.size() == 2u);
    assert(toc.sections[0].name == "TOKENS");
    assert(toc.sections[0].start_byte_offset == 0);
    assert(toc.sections[0].size_bytes == 0);
    assert(toc.sections[1].name == "PATHS");
    assert(toc.sections[1].start_byte_offset == 120);
    assert(toc.sections[1].size_bytes == 40);
    assert(!freeusd::usd::crate::ReadUsdCrateTocFromPath(p.string(), toc, 1, &err));
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
  assert(xf.ComputeLocalToWorldTransform(1.0) == freeusd::gf::Matrix4d::Identity());

  {
    freeusd::gf::Vec3d tw{};
    tw.set(10.0, 0.0, 0.0);
    freeusd::gf::Vec3d tc{};
    tc.set(1.0, 2.0, 3.0);
    layer->SetField(world, Token("xformOp:translate"), Value::MakeVec3d(tw));
    layer->SetField(cube, Token("xformOp:translate"), Value::MakeVec3d(tc));
    const freeusd::usdGeom::Xformable xf2(prim);
    const freeusd::gf::Matrix4d mw = xf2.ComputeLocalToWorldTransform(1.0);
    const freeusd::gf::Matrix4d expect =
        freeusd::gf::Matrix4d::Multiply(freeusd::gf::Matrix4d::Translate(10.0, 0.0, 0.0),
                                       freeusd::gf::Matrix4d::Translate(1.0, 2.0, 3.0));
    assert(mw == expect);
    layer->SetField(world, Token("xformOpOrder"), Value::MakeString("xformOp:translate,xformOp:scale"));
    freeusd::gf::Vec3d sw{};
    sw.set(2.0, 2.0, 2.0);
    layer->SetField(world, Token("xformOp:scale"), Value::MakeVec3d(sw));
    const freeusd::gf::Matrix4d mw2 = xf2.ComputeLocalToWorldTransform(1.0);
    const freeusd::gf::Matrix4d s = freeusd::gf::Matrix4d::Scale(2.0, 2.0, 2.0);
    const freeusd::gf::Matrix4d twm = freeusd::gf::Matrix4d::Translate(10.0, 0.0, 0.0);
    const freeusd::gf::Matrix4d tcm = freeusd::gf::Matrix4d::Translate(1.0, 2.0, 3.0);
    const freeusd::gf::Matrix4d expect2 = freeusd::gf::Matrix4d::Multiply(freeusd::gf::Matrix4d::Multiply(s, twm), tcm);
    assert(mw2 == expect2);
  }

  return 0;
}
