#include "freeusd/usd/crateFile.hpp"

#include <cstring>
#include <fstream>
#include <limits>
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

std::uint64_t readLeU64(const unsigned char* p) noexcept {
  std::uint64_t u = 0;
  for (int i = 0; i < 8; ++i) {
    u |= static_cast<std::uint64_t>(p[i]) << (8 * i);
  }
  return u;
}

constexpr std::size_t kTocSectionRecordBytes = 16u + sizeof(std::int64_t) + sizeof(std::int64_t);
static_assert(kTocSectionRecordBytes == 32u, "USDC TOC section record size");

/// ``[start, start+size)`` must lie in ``[0, file_bytes)``; rejects negative fields and ``int64`` overflow on ``+``.
bool toc_section_range_valid(std::int64_t file_bytes, std::int64_t start, std::int64_t size, std::string* err_out) {
  if (file_bytes < 0) {
    set_detail(err_out, "invalid file size for USDC TOC validation");
    return false;
  }
  if (start < 0) {
    set_detail(err_out, "USDC TOC section start_byte_offset is negative");
    return false;
  }
  if (size < 0) {
    set_detail(err_out, "USDC TOC section size_bytes is negative");
    return false;
  }
  if (size == 0) {
    if (start > file_bytes) {
      set_detail(err_out, "USDC TOC section start_byte_offset past end of file");
      return false;
    }
    return true;
  }
  const auto fb = static_cast<std::uint64_t>(file_bytes);
  const auto st = static_cast<std::uint64_t>(start);
  const auto sz = static_cast<std::uint64_t>(size);
  if (st > fb) {
    set_detail(err_out, "USDC TOC section start_byte_offset past end of file");
    return false;
  }
  if (sz > fb - st) {
    set_detail(err_out, "USDC TOC section byte range extends past end of file");
    return false;
  }
  return true;
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

bool ReadUsdCrateTocFromPath(const std::string& path, UsdcCrateToc& out, std::size_t max_sections,
                             std::string* err_out) {
  out = UsdcCrateToc{};
  if (path.empty()) {
    set_detail(err_out, "empty path");
    return false;
  }
  if (max_sections == 0) {
    set_detail(err_out, "max_sections must be non-zero");
    return false;
  }
  UsdcCrateBootstrap boot{};
  if (!ReadUsdCrateBootstrapFromPath(path, boot, err_out)) {
    return false;
  }
  std::ifstream in(path, std::ios::binary | std::ios::ate);
  if (!in) {
    set_detail(err_out, "could not open file");
    return false;
  }
  const auto end_pos = in.tellg();
  const std::int64_t file_bytes = static_cast<std::int64_t>(end_pos);
  const std::int64_t toc_off = boot.toc_byte_offset;
  if (toc_off < 0 || toc_off > file_bytes) {
    set_detail(err_out, "invalid TOC offset");
    return false;
  }
  const std::uint64_t bytes_after_toc =
      static_cast<std::uint64_t>(file_bytes) - static_cast<std::uint64_t>(toc_off);
  if (bytes_after_toc < 8u) {
    set_detail(err_out, "file too small for USDC TOC count");
    return false;
  }
  in.seekg(toc_off, std::ios::beg);
  unsigned char count_le[8];
  in.read(reinterpret_cast<char*>(count_le), static_cast<std::streamsize>(sizeof count_le));
  if (in.gcount() != static_cast<std::streamsize>(sizeof count_le)) {
    set_detail(err_out, "short read on USDC TOC count");
    return false;
  }
  const std::uint64_t n = readLeU64(count_le);
  if (n > max_sections) {
    set_detail(err_out, "USDC TOC section count exceeds max_sections");
    return false;
  }
  if (n > std::numeric_limits<std::uint64_t>::max() / kTocSectionRecordBytes) {
    set_detail(err_out, "USDC TOC section count overflow");
    return false;
  }
  const std::uint64_t toc_payload = 8u + n * kTocSectionRecordBytes;
  if (toc_payload > bytes_after_toc) {
    set_detail(err_out, "file too small for USDC TOC sections");
    return false;
  }
  out.section_count = n;
  out.sections.clear();
  out.sections.reserve(static_cast<std::size_t>(n));
  std::vector<unsigned char> rec(kTocSectionRecordBytes);
  for (std::uint64_t i = 0; i < n; ++i) {
    in.read(reinterpret_cast<char*>(rec.data()), static_cast<std::streamsize>(rec.size()));
    if (in.gcount() != static_cast<std::streamsize>(rec.size())) {
      set_detail(err_out, "short read on USDC TOC section");
      out = UsdcCrateToc{};
      return false;
    }
    UsdcCrateTocSection sec;
    std::size_t name_len = 0;
    while (name_len < 16u && rec[name_len] != '\0') {
      ++name_len;
    }
    sec.name.assign(reinterpret_cast<const char*>(rec.data()), name_len);
    sec.start_byte_offset = readLeI64(rec.data() + 16);
    sec.size_bytes = readLeI64(rec.data() + 24);
    if (!toc_section_range_valid(file_bytes, sec.start_byte_offset, sec.size_bytes, err_out)) {
      out = UsdcCrateToc{};
      return false;
    }
    out.sections.push_back(std::move(sec));
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
