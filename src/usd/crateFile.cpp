#include "freeusd/usd/crateFile.hpp"

#include "freeusd/ar/pathSecurity.hpp"

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

bool crate_file_size_ok(std::int64_t file_bytes, std::string* err_out) {
  if (file_bytes < 0) {
    set_detail(err_out, "invalid USDC file size");
    return false;
  }
  if (static_cast<std::uint64_t>(file_bytes) > freeusd::ar::kMaxUsdcCrateFileBytes) {
    set_detail(err_out, "USDC file exceeds maximum allowed size");
    return false;
  }
  return true;
}

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

bool readSizedStringTableSection(const std::string& path, std::string_view section_name, std::vector<std::string>* out,
                                 std::size_t max_entries, std::size_t max_total_bytes, std::string* err_out) {
  if (!out) {
    set_detail(err_out, "null output table");
    return false;
  }
  out->clear();
  if (max_entries == 0u) {
    set_detail(err_out, "max_entries must be non-zero");
    return false;
  }
  std::vector<std::uint8_t> bytes;
  if (!ReadUsdCrateSectionBytesFromPath(path, section_name, bytes, max_total_bytes, err_out)) {
    return false;
  }
  if (bytes.size() < 8u) {
    set_detail(err_out, "USDC table payload too small for count");
    return false;
  }
  const std::uint64_t count = readLeU64(bytes.data());
  if (count > max_entries) {
    set_detail(err_out, "USDC table entry count exceeds max_entries");
    return false;
  }
  std::size_t cursor = 8u;
  out->reserve(static_cast<std::size_t>(count));
  for (std::uint64_t i = 0; i < count; ++i) {
    if (cursor + 8u > bytes.size()) {
      out->clear();
      set_detail(err_out, "USDC table payload ended before string length");
      return false;
    }
    const std::uint64_t len = readLeU64(bytes.data() + cursor);
    cursor += 8u;
    if (len > static_cast<std::uint64_t>(bytes.size() - cursor)) {
      out->clear();
      set_detail(err_out, "USDC table string length exceeds payload bytes");
      return false;
    }
    out->emplace_back(reinterpret_cast<const char*>(bytes.data() + cursor), static_cast<std::size_t>(len));
    cursor += static_cast<std::size_t>(len);
  }
  if (cursor != bytes.size()) {
    set_detail(err_out, "USDC table payload has trailing bytes");
    return false;
  }
  return true;
}

bool readFieldsTableSection(const std::string& path, std::vector<UsdcCrateFieldEntry>* out, std::size_t max_entries,
                            std::size_t max_total_bytes, std::string* err_out) {
  if (!out) {
    set_detail(err_out, "null output table");
    return false;
  }
  out->clear();
  if (max_entries == 0u) {
    set_detail(err_out, "max_entries must be non-zero");
    return false;
  }
  std::vector<std::uint8_t> bytes;
  if (!ReadUsdCrateSectionBytesFromPath(path, "FIELDS", bytes, max_total_bytes, err_out)) {
    return false;
  }
  if (bytes.size() < 8u) {
    set_detail(err_out, "USDC FIELDS payload too small for count");
    return false;
  }
  const std::uint64_t count = readLeU64(bytes.data());
  if (count > max_entries) {
    set_detail(err_out, "USDC FIELDS entry count exceeds max_entries");
    return false;
  }
  constexpr std::size_t kFieldEntryBytes = 16u;
  const std::uint64_t need = 8u + count * kFieldEntryBytes;
  if (need > bytes.size()) {
    set_detail(err_out, "USDC FIELDS payload too small for entries");
    return false;
  }
  if (need != bytes.size()) {
    set_detail(err_out, "USDC FIELDS payload has trailing bytes");
    return false;
  }
  out->reserve(static_cast<std::size_t>(count));
  std::size_t cursor = 8u;
  for (std::uint64_t i = 0; i < count; ++i) {
    UsdcCrateFieldEntry entry;
    entry.token_index = readLeU64(bytes.data() + cursor);
    cursor += 8u;
    entry.value_type_token_index = readLeU64(bytes.data() + cursor);
    cursor += 8u;
    out->push_back(entry);
  }
  return true;
}

bool readSpecsTableSection(const std::string& path, std::vector<UsdcCrateSpecEntry>* out, std::size_t max_entries,
                           std::size_t max_total_bytes, std::string* err_out) {
  if (!out) {
    set_detail(err_out, "null output table");
    return false;
  }
  out->clear();
  if (max_entries == 0u) {
    set_detail(err_out, "max_entries must be non-zero");
    return false;
  }
  std::vector<std::uint8_t> bytes;
  if (!ReadUsdCrateSectionBytesFromPath(path, "SPECS", bytes, max_total_bytes, err_out)) {
    return false;
  }
  if (bytes.size() < 8u) {
    set_detail(err_out, "USDC SPECS payload too small for count");
    return false;
  }
  const std::uint64_t count = readLeU64(bytes.data());
  if (count > max_entries) {
    set_detail(err_out, "USDC SPECS entry count exceeds max_entries");
    return false;
  }
  constexpr std::size_t kSpecEntryBytes = 24u;
  const std::uint64_t need = 8u + count * kSpecEntryBytes;
  if (need > bytes.size()) {
    set_detail(err_out, "USDC SPECS payload too small for entries");
    return false;
  }
  if (need != bytes.size()) {
    set_detail(err_out, "USDC SPECS payload has trailing bytes");
    return false;
  }
  out->reserve(static_cast<std::size_t>(count));
  std::size_t cursor = 8u;
  for (std::uint64_t i = 0; i < count; ++i) {
    UsdcCrateSpecEntry entry;
    entry.path_index = readLeU64(bytes.data() + cursor);
    cursor += 8u;
    entry.field_set_index = readLeU64(bytes.data() + cursor);
    cursor += 8u;
    entry.spec_type = readLeU64(bytes.data() + cursor);
    cursor += 8u;
    out->push_back(entry);
  }
  return true;
}

bool readFieldSetsTableSection(const std::string& path, std::vector<UsdcCrateFieldSet>* out, std::size_t max_field_sets,
                               std::size_t max_fields_per_set, std::size_t max_total_bytes, std::string* err_out) {
  if (!out) {
    set_detail(err_out, "null output table");
    return false;
  }
  out->clear();
  if (max_field_sets == 0u || max_fields_per_set == 0u) {
    set_detail(err_out, "max_field_sets and max_fields_per_set must be non-zero");
    return false;
  }
  std::vector<std::uint8_t> bytes;
  if (!ReadUsdCrateSectionBytesFromPath(path, "FIELDSETS", bytes, max_total_bytes, err_out)) {
    return false;
  }
  if (bytes.size() < 8u) {
    set_detail(err_out, "USDC FIELDSETS payload too small for count");
    return false;
  }
  const std::uint64_t set_count = readLeU64(bytes.data());
  if (set_count > max_field_sets) {
    set_detail(err_out, "USDC FIELDSETS set count exceeds max_field_sets");
    return false;
  }
  std::size_t cursor = 8u;
  out->reserve(static_cast<std::size_t>(set_count));
  for (std::uint64_t s = 0; s < set_count; ++s) {
    if (cursor + 8u > bytes.size()) {
      out->clear();
      set_detail(err_out, "USDC FIELDSETS payload ended before field count");
      return false;
    }
    const std::uint64_t field_count = readLeU64(bytes.data() + cursor);
    cursor += 8u;
    if (field_count > max_fields_per_set) {
      out->clear();
      set_detail(err_out, "USDC FIELDSETS field count exceeds max_fields_per_set");
      return false;
    }
    const std::uint64_t need = field_count * 8u;
    if (cursor + need > bytes.size()) {
      out->clear();
      set_detail(err_out, "USDC FIELDSETS payload too small for field indices");
      return false;
    }
    UsdcCrateFieldSet set;
    set.field_indices.reserve(static_cast<std::size_t>(field_count));
    for (std::uint64_t f = 0; f < field_count; ++f) {
      set.field_indices.push_back(readLeU64(bytes.data() + cursor));
      cursor += 8u;
    }
    out->push_back(std::move(set));
  }
  if (cursor != bytes.size()) {
    out->clear();
    set_detail(err_out, "USDC FIELDSETS payload has trailing bytes");
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
  if (!crate_file_size_ok(file_bytes, err_out)) {
    return false;
  }
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
  if (!crate_file_size_ok(file_bytes, err_out)) {
    return false;
  }
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

bool ReadUsdCrateSectionBytesFromPath(const std::string& path, std::string_view section_name,
                                      std::vector<std::uint8_t>& out_bytes, std::size_t max_section_bytes,
                                      std::string* err_out) {
  out_bytes.clear();
  if (path.empty()) {
    set_detail(err_out, "empty path");
    return false;
  }
  if (section_name.empty()) {
    set_detail(err_out, "empty USDC section name");
    return false;
  }
  if (section_name.size() > 16u) {
    set_detail(err_out, "USDC section name exceeds 16-byte TOC entry width");
    return false;
  }

  UsdcCrateToc toc{};
  if (!ReadUsdCrateTocFromPath(path, toc, 65536u, err_out)) {
    return false;
  }

  const UsdcCrateTocSection* match = nullptr;
  for (const auto& sec : toc.sections) {
    if (sec.name == section_name) {
      match = &sec;
      break;
    }
  }
  if (!match) {
    set_detail(err_out, "USDC TOC section not found");
    return false;
  }
  if (match->size_bytes < 0) {
    set_detail(err_out, "USDC TOC section size_bytes is negative");
    return false;
  }
  const std::uint64_t want = static_cast<std::uint64_t>(match->size_bytes);
  if (want > static_cast<std::uint64_t>(max_section_bytes)) {
    set_detail(err_out, "USDC section size exceeds max_section_bytes");
    return false;
  }
  if (want == 0u) {
    return true;
  }

  std::ifstream in(path, std::ios::binary);
  if (!in) {
    set_detail(err_out, "could not open file");
    return false;
  }
  in.seekg(match->start_byte_offset, std::ios::beg);
  if (!in) {
    set_detail(err_out, "failed to seek to USDC section payload");
    return false;
  }
  out_bytes.resize(static_cast<std::size_t>(want));
  in.read(reinterpret_cast<char*>(out_bytes.data()), static_cast<std::streamsize>(out_bytes.size()));
  if (in.gcount() != static_cast<std::streamsize>(out_bytes.size())) {
    out_bytes.clear();
    set_detail(err_out, "short read on USDC section payload");
    return false;
  }
  return true;
}

bool ReadUsdCrateUsdaSectionFromPath(const std::string& path, std::string& out_text, std::size_t max_text_bytes,
                                     std::string* err_out) {
  out_text.clear();
  std::vector<std::uint8_t> bytes;
  if (!ReadUsdCrateSectionBytesFromPath(path, "USDA", bytes, max_text_bytes, err_out)) {
    return false;
  }
  out_text.assign(reinterpret_cast<const char*>(bytes.data()), bytes.size());
  return true;
}

bool ReadUsdCrateStringTableFromPath(const std::string& path, UsdcCrateStringTable& out, std::size_t max_entries,
                                     std::size_t max_total_bytes, std::string* err_out) {
  out = UsdcCrateStringTable{};
  return readSizedStringTableSection(path, "STRINGS", &out.values, max_entries, max_total_bytes, err_out);
}

bool ReadUsdCrateTokenTableFromPath(const std::string& path, UsdcCrateStringTable& out, std::size_t max_entries,
                                    std::size_t max_total_bytes, std::string* err_out) {
  out = UsdcCrateStringTable{};
  return readSizedStringTableSection(path, "TOKENS", &out.values, max_entries, max_total_bytes, err_out);
}

bool ReadUsdCratePathTableFromPath(const std::string& path, UsdcCratePathTable& out, std::size_t max_entries,
                                   std::size_t max_total_bytes, std::string* err_out) {
  out = UsdcCratePathTable{};
  std::vector<std::string> items;
  if (!readSizedStringTableSection(path, "PATHS", &items, max_entries, max_total_bytes, err_out)) {
    return false;
  }
  out.paths.reserve(items.size());
  for (const std::string& item : items) {
    const freeusd::sdf::Path path_value = freeusd::sdf::Path::FromString(item);
    if (path_value.IsEmpty()) {
      out = UsdcCratePathTable{};
      set_detail(err_out, "USDC PATHS entry is not a valid sdf path");
      return false;
    }
    out.paths.push_back(path_value);
  }
  return true;
}

bool ReadUsdCrateFieldsTableFromPath(const std::string& path, UsdcCrateFieldsTable& out, std::size_t max_entries,
                                    std::size_t max_total_bytes, std::string* err_out) {
  out = UsdcCrateFieldsTable{};
  return readFieldsTableSection(path, &out.entries, max_entries, max_total_bytes, err_out);
}

bool ReadUsdCrateSpecsTableFromPath(const std::string& path, UsdcCrateSpecsTable& out, std::size_t max_entries,
                                    std::size_t max_total_bytes, std::string* err_out) {
  out = UsdcCrateSpecsTable{};
  return readSpecsTableSection(path, &out.entries, max_entries, max_total_bytes, err_out);
}

bool ReadUsdCrateFieldSetsTableFromPath(const std::string& path, UsdcCrateFieldSetsTable& out,
                                        std::size_t max_field_sets, std::size_t max_fields_per_set,
                                        std::size_t max_total_bytes, std::string* err_out) {
  out = UsdcCrateFieldSetsTable{};
  return readFieldSetsTableSection(path, &out.sets, max_field_sets, max_fields_per_set, max_total_bytes, err_out);
}

bool ReadUsdCrateValuesTableFromPath(const std::string& path, UsdcCrateValuesTable& out, std::size_t max_entries,
                                     std::size_t max_total_bytes, std::string* err_out) {
  out = UsdcCrateValuesTable{};
  if (max_entries == 0u) {
    set_detail(err_out, "max_entries must be non-zero");
    return false;
  }
  std::vector<std::string> blobs;
  if (!readSizedStringTableSection(path, "VALUES", &blobs, max_entries, max_total_bytes, err_out)) {
    return false;
  }
  out.entries.reserve(blobs.size());
  for (std::string& blob : blobs) {
    UsdcCrateValueEntry entry;
    entry.bytes.assign(blob.begin(), blob.end());
    out.entries.push_back(std::move(entry));
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
