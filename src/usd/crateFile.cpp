#include "freeusd/usd/crateFile.hpp"

#include <fstream>
#include <vector>

namespace freeusd::usd::crate {

namespace {

void set_detail(std::string* detail_out, std::string msg) {
  if (detail_out) {
    *detail_out = std::move(msg);
  }
}

}  // namespace

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
