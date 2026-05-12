#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <random>
#include <string>
#include <vector>

#include "freeusd/usd/crateFile.hpp"
#include "freeusd/sdf/path.hpp"
#include "freeusd/usd/stage.hpp"

namespace {

void write_le_u64(unsigned char* dst, std::uint64_t value) {
  for (int i = 0; i < 8; ++i) {
    dst[i] = static_cast<unsigned char>((value >> (8 * i)) & 0xffu);
  }
}

void write_le_i64(unsigned char* dst, std::int64_t value) {
  write_le_u64(dst, static_cast<std::uint64_t>(value));
}

std::vector<unsigned char> make_embedded_usda_crate(const std::string& usda_text) {
  const std::size_t header_bytes = 88u;
  const std::size_t toc_offset = header_bytes + usda_text.size();
  const std::size_t total_bytes = toc_offset + 8u + 32u;
  std::vector<unsigned char> bytes(total_bytes, 0);
  const std::string magic = std::string(freeusd::usd::crate::UsdcCrateIdentifier());
  std::copy(magic.begin(), magic.end(), bytes.begin());
  bytes[8] = 0;
  bytes[9] = 8;
  bytes[10] = 0;
  write_le_i64(bytes.data() + 16u, static_cast<std::int64_t>(toc_offset));
  std::copy(usda_text.begin(), usda_text.end(), bytes.begin() + static_cast<std::ptrdiff_t>(header_bytes));
  write_le_u64(bytes.data() + static_cast<std::ptrdiff_t>(toc_offset), 1u);
  std::memcpy(bytes.data() + static_cast<std::ptrdiff_t>(toc_offset + 8u), "USDA", 4u);
  write_le_i64(bytes.data() + static_cast<std::ptrdiff_t>(toc_offset + 24u), static_cast<std::int64_t>(header_bytes));
  write_le_i64(bytes.data() + static_cast<std::ptrdiff_t>(toc_offset + 32u), static_cast<std::int64_t>(usda_text.size()));
  return bytes;
}

}  // namespace

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

  const fs::path crate_path = base / "embedded.usdc";
  {
    const std::string embedded_usda = "#usda 1.0\n(\n    defaultPrim = \"Root\"\n)\ndef Xform \"Root\"\n{\n}\n";
    const std::vector<unsigned char> crate = make_embedded_usda_crate(embedded_usda);
    std::ofstream out(crate_path, std::ios::binary);
    out.write(reinterpret_cast<const char*>(crate.data()), static_cast<std::streamsize>(crate.size()));
  }
  auto crate_stage = Stage::OpenFromRootFile(crate_path.string(), RootLayerSublayersPolicy::None, &err);
  assert(crate_stage);
  assert(crate_stage->HasDefaultPrim());
  assert(crate_stage->GetDefaultPrimName() == "Root");
  assert(crate_stage->PrimPathInUse(Path::FromString("/Root")));

  fs::remove_all(base);
  return 0;
}
