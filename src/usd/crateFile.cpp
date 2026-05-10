#include "freeusd/usd/crateFile.hpp"

#include <cstring>
#include <fstream>
#include <vector>

namespace freeusd::usd::crate {

namespace {

void set_detail(std::string* detail_out, std::string msg) {
  if (detail_out) {
    *detail_out = std::move(msg);
  }
}

constexpr std::size_t kCrateBootStrapBytes = 8 + 8 + sizeof(std::int64_t) + 8 * sizeof(std::int64_t);
static_assert(kCrateBootStrapBytes == 88u, "USDC bootstrap size");

std::int64_t readLeI64(const unsigned char* p) noexcept {
  std::uint64_t u = 0;
  for (int i = 0; i < 8; ++i) {
    u |= static_cast<std::uint64_t>(p[i]) << (8 * i);
  }
  std::int64_t v = 0;
  std::memcpy(&v, &u, sizeof(v));
  return v;
}

}  // namespace

bool ReadUsdCrateBootstrapFromPath(const std::string& path, UsdcCrateBootstrap& out, std::string* err_out) {
  out = UsdcCrateBootstrap{};
  if (path.empty()) {
    set_detail(err_out, "empty path");
    return false;
  }
  std::ifstream in(path, std::ios::binary | std::ios::ate);
  if (!in) {
    set_detail(err_out, "could not open file");
    return false;
  }
  const auto end_pos = in.tellg();
  const std::int64_t file_bytes = static_cast<std::int64_t>(end_pos);
  if (file_bytes < static_cast<std::int64_t>(kCrateBootStrapBytes)) {
    set_detail(err_out, "file too small for USDC bootstrap");
    return false;
  }
  in.seekg(0, std::ios::beg);
  std::vector<unsigned char> buf(kCrateBootStrapBytes);
  in.read(reinterpret_cast<char*>(buf.data()), static_cast<std::streamsize>(buf.size()));
  if (in.gcount() != static_cast<std::streamsize>(buf.size())) {
    set_detail(err_out, "short read on bootstrap");
    return false;
  }
  const std::string_view magic = UsdcCrateIdentifier();
  if (buf.size() < magic.size() || std::memcmp(buf.data(), magic.data(), magic.size()) != 0) {
    set_detail(err_out, "missing PXR-USDC magic");
    return false;
  }
  out.file_version_major = buf[8];
  out.file_version_minor = buf[9];
  out.file_version_patch = buf[10];
  out.toc_byte_offset = readLeI64(buf.data() + 16);
  if (out.toc_byte_offset < static_cast<std::int64_t>(kCrateBootStrapBytes)) {
    set_detail(err_out, "table-of-contents offset precedes bootstrap");
    return false;
  }
  if (out.toc_byte_offset >= file_bytes) {
    set_detail(err_out, "table-of-contents offset past end of file");
    return false;
  }
  return true;
}

UsdFileKind DetectUsdFileKindFromPath(const std::string& path, std::string* detail_out) {
  if (path.empty()) {
    set_detail(detail_out, "empty path");
    return UsdFileKind::IoOrEmpty;
  }
  std::ifstream in(path, std::ios::binary);
  if (!in) {
    set_detail(detail_out, "could not open file");
    return UsdFileKind::IoOrEmpty;
  }
  constexpr std::size_t kBuf = 64;
  std::vector<char> buf(kBuf);
  in.read(buf.data(), static_cast<std::streamsize>(buf.size()));
  const std::streamsize got = in.gcount();
  if (got <= 0) {
    set_detail(detail_out, "empty file");
    return UsdFileKind::IoOrEmpty;
  }
  const std::string_view head(buf.data(), static_cast<std::size_t>(got));
  const std::string_view magic = UsdcCrateIdentifier();
  if (head.size() >= magic.size() && head.compare(0, magic.size(), magic) == 0) {
    return UsdFileKind::UsdcCrate;
  }
  // USDA layer files conventionally start with "#usda 1.0" (allow leading BOM or whitespace).
  std::size_t i = 0;
  while (i < head.size() && (head[i] == ' ' || head[i] == '\t' || head[i] == '\r' || head[i] == '\n')) {
    ++i;
  }
  if (i < head.size() && head[i] == '\xef') {
    // UTF-8 BOM
    if (i + 2 < head.size() && static_cast<unsigned char>(head[i + 1]) == 0xbb &&
        static_cast<unsigned char>(head[i + 2]) == 0xbf) {
      i += 3;
    }
  }
  const std::string_view trimmed = head.substr(i);
  if (trimmed.size() >= 9 && trimmed.compare(0, 9, "#usda 1.0") == 0) {
    return UsdFileKind::UsdaAscii;
  }
  if (trimmed.size() >= 5 && trimmed.compare(0, 5, "#usda") == 0) {
    return UsdFileKind::UsdaAscii;
  }
  return UsdFileKind::Unknown;
}

}  // namespace freeusd::usd::crate
