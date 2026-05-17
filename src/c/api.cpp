#include "freeusd/c/freeusd.h"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "freeusd/gf/matrix4d.hpp"
#include "freeusd/gf/quatd.hpp"
#include "freeusd/gf/quatf.hpp"
#include "freeusd/gf/vec3d.hpp"
#include "freeusd/gf/vec3f.hpp"
#include "freeusd/io/usda.hpp"
#include "freeusd/pcp/layerStack.hpp"
#include "freeusd/sdf/layer.hpp"
#include "freeusd/sdf/path.hpp"
#include "freeusd/sdf/primReference.hpp"
#include "freeusd/tf/token.hpp"
#include "freeusd/usd/crateFile.hpp"
#include "freeusd/usd/prim.hpp"
#include "freeusd/usd/stage.hpp"
#include "freeusd/usdGeom/boundable.hpp"
#include "freeusd/usdGeom/imageable.hpp"
#include "freeusd/usdGeom/xformable.hpp"
#include "freeusd/usdSkel/skelAnimation.hpp"
#include "freeusd/usdSkel/skelBlendShapes.hpp"
#include "freeusd/usdSkel/skinning.hpp"
#include "freeusd/usdSkel/skeleton.hpp"
#include "freeusd/usdShade/material.hpp"
#include "freeusd/usdShade/previewSurface.hpp"
#include "freeusd/usdUtils/engineScene.hpp"
#include "freeusd/version.hpp"
#include "freeusd/vt/value.hpp"

namespace {

thread_local std::string g_last_error;

void clear_error() { g_last_error.clear(); }

void set_error(std::string msg) { g_last_error = std::move(msg); }

char* dup_cstr(const std::string& s) {
  char* p = static_cast<char*>(std::malloc(s.size() + 1));
  if (!p) {
    return nullptr;
  }
  std::memcpy(p, s.c_str(), s.size() + 1);
  return p;
}

/** On @ref FREEUSD_OK, @p *out_arr / @p *out_count follow @ref freeusd_path_list_free ownership. */
int malloc_string_list(const std::vector<std::string>& items, char*** out_arr, size_t* out_count) {
  if (!out_arr || !out_count) {
    return FREEUSD_ERR_INVALID_ARGUMENT;
  }
  *out_arr = nullptr;
  *out_count = 0;
  if (items.empty()) {
    return FREEUSD_OK;
  }
  char** arr = static_cast<char**>(std::malloc(items.size() * sizeof(char*)));
  if (!arr) {
    return FREEUSD_ERR_INTERNAL;
  }
  for (std::size_t i = 0; i < items.size(); ++i) {
    arr[i] = dup_cstr(items[i]);
    if (!arr[i]) {
      for (std::size_t j = 0; j < i; ++j) {
        std::free(arr[j]);
      }
      std::free(arr);
      return FREEUSD_ERR_INTERNAL;
    }
  }
  *out_arr = arr;
  *out_count = items.size();
  return FREEUSD_OK;
}

bool value_to_int64(const freeusd::vt::Value& v, std::int64_t* out) {
  if (!out) {
    return false;
  }
  const auto& p = v.GetPayload();
  if (const bool* b = std::get_if<bool>(&p)) {
    *out = *b ? 1 : 0;
    return true;
  }
  if (const std::int32_t* i32 = std::get_if<std::int32_t>(&p)) {
    *out = *i32;
    return true;
  }
  if (const std::int64_t* i64 = std::get_if<std::int64_t>(&p)) {
    *out = *i64;
    return true;
  }
  if (const float* f = std::get_if<float>(&p)) {
    *out = static_cast<std::int64_t>(*f);
    return true;
  }
  if (const double* d = std::get_if<double>(&p)) {
    *out = static_cast<std::int64_t>(*d);
    return true;
  }
  return false;
}

}  // namespace

struct FreeusdLayer {
  std::shared_ptr<freeusd::sdf::Layer> inner;
};

struct FreeusdLayerStack {
  freeusd::pcp::LayerStack inner;
};

struct FreeusdStage {
  std::shared_ptr<freeusd::usd::Stage> inner;
};

extern "C" {

const char* freeusd_version_string(void) {
  static const std::string s = freeusd::version_string();
  return s.c_str();
}

const char* freeusd_usdc_crate_identifier_utf8(void) {
  static const std::string s = std::string{freeusd::usd::crate::UsdcCrateIdentifier()};
  return s.c_str();
}

int freeusd_detect_usd_file_kind_from_path_utf8(const char* path_utf8, int* out_kind, char** out_detail_utf8) {
  if (!path_utf8 || !out_kind) {
    return FREEUSD_ERR_INVALID_ARGUMENT;
  }
  if (out_detail_utf8) {
    *out_detail_utf8 = nullptr;
  }
  try {
    clear_error();
    std::string detail;
    std::string* detail_ptr = out_detail_utf8 ? &detail : nullptr;
    const auto k = freeusd::usd::crate::DetectUsdFileKindFromPath(std::string{path_utf8}, detail_ptr);
    *out_kind = static_cast<int>(k);
    if (out_detail_utf8 && !detail.empty()) {
      *out_detail_utf8 = dup_cstr(detail);
      if (!*out_detail_utf8) {
        return FREEUSD_ERR_INTERNAL;
      }
    }
    return FREEUSD_OK;
  } catch (const std::exception& e) {
    set_error(e.what());
    return FREEUSD_ERR_INTERNAL;
  } catch (...) {
    set_error("unknown error");
    return FREEUSD_ERR_INTERNAL;
  }
}

int freeusd_read_usdc_bootstrap_from_path_utf8(const char* path_utf8, FreeusdUsdcBootstrap* out_bootstrap) {
  if (!path_utf8 || !out_bootstrap) {
    return FREEUSD_ERR_INVALID_ARGUMENT;
  }
  std::memset(out_bootstrap, 0, sizeof(*out_bootstrap));
  try {
    clear_error();
    freeusd::usd::crate::UsdcCrateBootstrap b;
    std::string err;
    if (!freeusd::usd::crate::ReadUsdCrateBootstrapFromPath(std::string{path_utf8}, b, &err)) {
      set_error(err.empty() ? "read usdc bootstrap failed" : err);
      return FREEUSD_ERR_PARSE;
    }
    out_bootstrap->file_version_major = b.file_version_major;
    out_bootstrap->file_version_minor = b.file_version_minor;
    out_bootstrap->file_version_patch = b.file_version_patch;
    out_bootstrap->toc_byte_offset = b.toc_byte_offset;
    return FREEUSD_OK;
  } catch (const std::exception& e) {
    set_error(e.what());
    return FREEUSD_ERR_INTERNAL;
  } catch (...) {
    set_error("unknown error");
    return FREEUSD_ERR_INTERNAL;
  }
}

int freeusd_read_usdc_toc_from_path_utf8(const char* path_utf8, uint64_t max_sections, uint64_t* out_total_section_count,
                                         FreeusdUsdcTocSection** out_sections, uint64_t* out_sections_returned) {
  if (!path_utf8 || !out_total_section_count || !out_sections || !out_sections_returned) {
    return FREEUSD_ERR_INVALID_ARGUMENT;
  }
  *out_total_section_count = 0;
  *out_sections = nullptr;
  *out_sections_returned = 0;
  if (max_sections == 0) {
    return FREEUSD_ERR_INVALID_ARGUMENT;
  }
  try {
    clear_error();
    freeusd::usd::crate::UsdcCrateToc toc;
    std::string err;
    if (!freeusd::usd::crate::ReadUsdCrateTocFromPath(std::string{path_utf8}, toc,
                                                       static_cast<std::size_t>(max_sections), &err)) {
      set_error(err.empty() ? "read usdc toc failed" : err);
      return FREEUSD_ERR_PARSE;
    }
    *out_total_section_count = toc.section_count;
    *out_sections_returned = toc.section_count;
    if (toc.section_count == 0) {
      *out_sections = nullptr;
      return FREEUSD_OK;
    }
    auto* arr = static_cast<FreeusdUsdcTocSection*>(
        std::malloc(static_cast<std::size_t>(toc.section_count) * sizeof(FreeusdUsdcTocSection)));
    if (!arr) {
      set_error("out of memory");
      return FREEUSD_ERR_INTERNAL;
    }
    for (std::uint64_t i = 0; i < toc.section_count; ++i) {
      std::memset(&arr[i], 0, sizeof(arr[i]));
      const auto& s = toc.sections[static_cast<std::size_t>(i)];
      const std::size_t n = std::min(s.name.size(), sizeof(arr[i].name) - 1u);
      if (n > 0) {
        std::memcpy(arr[i].name, s.name.data(), n);
      }
      arr[i].start_byte_offset = s.start_byte_offset;
      arr[i].size_bytes = s.size_bytes;
    }
    *out_sections = arr;
    return FREEUSD_OK;
  } catch (const std::exception& e) {
    set_error(e.what());
    return FREEUSD_ERR_INTERNAL;
  } catch (...) {
    set_error("unknown error");
    return FREEUSD_ERR_INTERNAL;
  }
}

void freeusd_usdc_toc_sections_free(FreeusdUsdcTocSection* sections) { std::free(sections); }

int freeusd_read_usdc_section_bytes_from_path_utf8(const char* path_utf8, const char* section_name_utf8,
                                                   uint64_t max_bytes, uint8_t** out_bytes, uint64_t* out_size) {
  if (!path_utf8 || !section_name_utf8 || !out_bytes || !out_size) {
    return FREEUSD_ERR_INVALID_ARGUMENT;
  }
  *out_bytes = nullptr;
  *out_size = 0;
  if (section_name_utf8[0] == '\0') {
    return FREEUSD_ERR_INVALID_ARGUMENT;
  }
  try {
    clear_error();
    std::vector<std::uint8_t> bytes;
    std::string err;
    if (!freeusd::usd::crate::ReadUsdCrateSectionBytesFromPath(std::string{path_utf8}, std::string_view{section_name_utf8},
                                                               bytes, static_cast<std::size_t>(max_bytes), &err)) {
      set_error(err.empty() ? "read usdc section payload failed" : err);
      if (err.find("not found") != std::string::npos) {
        return FREEUSD_ERR_NOT_FOUND;
      }
      return FREEUSD_ERR_PARSE;
    }
    *out_size = static_cast<uint64_t>(bytes.size());
    if (bytes.empty()) {
      return FREEUSD_OK;
    }
    auto* arr = static_cast<uint8_t*>(std::malloc(bytes.size()));
    if (!arr) {
      set_error("out of memory");
      return FREEUSD_ERR_INTERNAL;
    }
    std::memcpy(arr, bytes.data(), bytes.size());
    *out_bytes = arr;
    return FREEUSD_OK;
  } catch (const std::exception& e) {
    set_error(e.what());
    return FREEUSD_ERR_INTERNAL;
  } catch (...) {
    set_error("unknown error");
    return FREEUSD_ERR_INTERNAL;
  }
}

void freeusd_bytes_free(void* bytes) { std::free(bytes); }

int freeusd_read_usdc_token_table_from_path_utf8(const char* path_utf8, uint64_t max_entries, uint64_t max_total_bytes,
                                                 char*** out_strings, size_t* out_count) {
  if (!path_utf8 || !out_strings || !out_count || max_entries == 0u) {
    return FREEUSD_ERR_INVALID_ARGUMENT;
  }
  try {
    clear_error();
    freeusd::usd::crate::UsdcCrateStringTable table;
    std::string err;
    if (!freeusd::usd::crate::ReadUsdCrateTokenTableFromPath(std::string{path_utf8}, table,
                                                             static_cast<std::size_t>(max_entries),
                                                             static_cast<std::size_t>(max_total_bytes), &err)) {
      set_error(err.empty() ? "read usdc token table failed" : err);
      return FREEUSD_ERR_PARSE;
    }
    return malloc_string_list(table.values, out_strings, out_count);
  } catch (const std::exception& e) {
    set_error(e.what());
    return FREEUSD_ERR_INTERNAL;
  } catch (...) {
    set_error("unknown error");
    return FREEUSD_ERR_INTERNAL;
  }
}

int freeusd_read_usdc_string_table_from_path_utf8(const char* path_utf8, uint64_t max_entries, uint64_t max_total_bytes,
                                                  char*** out_strings, size_t* out_count) {
  if (!path_utf8 || !out_strings || !out_count || max_entries == 0u) {
    return FREEUSD_ERR_INVALID_ARGUMENT;
  }
  try {
    clear_error();
    freeusd::usd::crate::UsdcCrateStringTable table;
    std::string err;
    if (!freeusd::usd::crate::ReadUsdCrateStringTableFromPath(std::string{path_utf8}, table,
                                                              static_cast<std::size_t>(max_entries),
                                                              static_cast<std::size_t>(max_total_bytes), &err)) {
      set_error(err.empty() ? "read usdc string table failed" : err);
      return FREEUSD_ERR_PARSE;
    }
    return malloc_string_list(table.values, out_strings, out_count);
  } catch (const std::exception& e) {
    set_error(e.what());
    return FREEUSD_ERR_INTERNAL;
  } catch (...) {
    set_error("unknown error");
    return FREEUSD_ERR_INTERNAL;
  }
}

int freeusd_read_usdc_path_table_from_path_utf8(const char* path_utf8, uint64_t max_entries, uint64_t max_total_bytes,
                                                char*** out_paths, size_t* out_count) {
  if (!path_utf8 || !out_paths || !out_count || max_entries == 0u) {
    return FREEUSD_ERR_INVALID_ARGUMENT;
  }
  try {
    clear_error();
    freeusd::usd::crate::UsdcCratePathTable table;
    std::string err;
    if (!freeusd::usd::crate::ReadUsdCratePathTableFromPath(std::string{path_utf8}, table,
                                                            static_cast<std::size_t>(max_entries),
                                                            static_cast<std::size_t>(max_total_bytes), &err)) {
      set_error(err.empty() ? "read usdc path table failed" : err);
      return FREEUSD_ERR_PARSE;
    }
    std::vector<std::string> items;
    items.reserve(table.paths.size());
    for (const freeusd::sdf::Path& path : table.paths) {
      items.push_back(path.GetString());
    }
    return malloc_string_list(items, out_paths, out_count);
  } catch (const std::exception& e) {
    set_error(e.what());
    return FREEUSD_ERR_INTERNAL;
  } catch (...) {
    set_error("unknown error");
    return FREEUSD_ERR_INTERNAL;
  }
}

const char* freeusd_last_error_message(void) { return g_last_error.c_str(); }

void freeusd_string_free(char* s) { std::free(s); }

FreeusdLayer* freeusd_layer_new_anonymous(const char* identifier) {
  try {
    clear_error();
    const std::string id = identifier ? std::string{identifier} : std::string{};
    auto sp = freeusd::sdf::Layer::NewAnonymous(id);
    return new FreeusdLayer{std::move(sp)};
  } catch (const std::exception& e) {
    set_error(e.what());
    return nullptr;
  } catch (...) {
    set_error("unknown exception");
    return nullptr;
  }
}

void freeusd_layer_free(FreeusdLayer* layer) {
  clear_error();
  delete layer;
}

int freeusd_layer_load_usda_from_string(FreeusdLayer* layer, const char* usda_text) {
  if (!layer || !layer->inner || !usda_text) {
    set_error("freeusd_layer_load_usda_from_string: null argument");
    return FREEUSD_ERR_INVALID_ARGUMENT;
  }
  try {
    const freeusd::io::usda::ParseResult r = freeusd::io::usda::LoadFromString(usda_text, layer->inner);
    if (!r.ok) {
      set_error(r.message.empty() ? "USDA parse failed" : r.message);
      return FREEUSD_ERR_PARSE;
    }
    clear_error();
    return FREEUSD_OK;
  } catch (const std::exception& e) {
    set_error(e.what());
    return FREEUSD_ERR_INTERNAL;
  } catch (...) {
    set_error("unknown exception");
    return FREEUSD_ERR_INTERNAL;
  }
}

char* freeusd_layer_save_usda_to_string(const FreeusdLayer* layer) {
  if (!layer || !layer->inner) {
    set_error("freeusd_layer_save_usda_to_string: null layer");
    return nullptr;
  }
  try {
    const std::string s = freeusd::io::usda::SaveToString(*layer->inner);
    clear_error();
    return dup_cstr(s);
  } catch (const std::exception& e) {
    set_error(e.what());
    return nullptr;
  } catch (...) {
    set_error("unknown exception");
    return nullptr;
  }
}

char* freeusd_layer_get_identifier_utf8(const FreeusdLayer* layer) {
  if (!layer || !layer->inner) {
    set_error("freeusd_layer_get_identifier_utf8: null layer");
    return nullptr;
  }
  try {
    const std::string& id = layer->inner->GetIdentifier();
    char* out = dup_cstr(id);
    if (!out) {
      set_error("out of memory");
      return nullptr;
    }
    clear_error();
    return out;
  } catch (const std::exception& e) {
    set_error(e.what());
    return nullptr;
  } catch (...) {
    set_error("unknown exception");
    return nullptr;
  }
}

int freeusd_layer_list_prim_paths(const FreeusdLayer* layer, char*** out_paths, size_t* out_count) {
  if (!layer || !layer->inner || !out_paths || !out_count) {
    set_error("freeusd_layer_list_prim_paths: null argument");
    return FREEUSD_ERR_INVALID_ARGUMENT;
  }
  *out_paths = nullptr;
  *out_count = 0;
  try {
    std::vector<freeusd::sdf::Path> paths = layer->inner->ListPrimPaths();
    std::vector<std::string> utf8;
    utf8.reserve(paths.size());
    for (const freeusd::sdf::Path& q : paths) {
      utf8.push_back(q.GetString());
    }
    std::sort(utf8.begin(), utf8.end());
    if (utf8.empty()) {
      clear_error();
      return FREEUSD_OK;
    }
    char** arr = static_cast<char**>(std::malloc(utf8.size() * sizeof(char*)));
    if (!arr) {
      set_error("out of memory");
      return FREEUSD_ERR_INTERNAL;
    }
    for (std::size_t i = 0; i < utf8.size(); ++i) {
      arr[i] = dup_cstr(utf8[i]);
      if (!arr[i]) {
        for (std::size_t j = 0; j < i; ++j) {
          std::free(arr[j]);
        }
        std::free(arr);
        set_error("out of memory");
        return FREEUSD_ERR_INTERNAL;
      }
    }
    *out_paths = arr;
    *out_count = utf8.size();
    clear_error();
    return FREEUSD_OK;
  } catch (const std::exception& e) {
    set_error(e.what());
    return FREEUSD_ERR_INTERNAL;
  } catch (...) {
    set_error("unknown exception");
    return FREEUSD_ERR_INTERNAL;
  }
}

char* freeusd_layer_get_documentation_utf8(const FreeusdLayer* layer) {
  if (!layer || !layer->inner) {
    set_error("freeusd_layer_get_documentation_utf8: null layer");
    return nullptr;
  }
  try {
    const std::string& doc = layer->inner->GetDocumentation();
    char* out = dup_cstr(doc);
    if (!out) {
      set_error("out of memory");
      return nullptr;
    }
    clear_error();
    return out;
  } catch (const std::exception& e) {
    set_error(e.what());
    return nullptr;
  } catch (...) {
    set_error("unknown exception");
    return nullptr;
  }
}

int freeusd_layer_has_default_prim(const FreeusdLayer* layer) {
  if (!layer || !layer->inner) {
    set_error("freeusd_layer_has_default_prim: null layer");
    return FREEUSD_ERR_INVALID_ARGUMENT;
  }
  try {
    const int out = layer->inner->HasDefaultPrim() ? 1 : 0;
    clear_error();
    return out;
  } catch (const std::exception& e) {
    set_error(e.what());
    return FREEUSD_ERR_INTERNAL;
  } catch (...) {
    set_error("unknown exception");
    return FREEUSD_ERR_INTERNAL;
  }
}

int freeusd_layer_get_default_prim_utf8(const FreeusdLayer* layer, char** out_prim_name) {
  if (!layer || !layer->inner || !out_prim_name) {
    set_error("freeusd_layer_get_default_prim_utf8: null argument");
    return FREEUSD_ERR_INVALID_ARGUMENT;
  }
  *out_prim_name = nullptr;
  try {
    if (!layer->inner->HasDefaultPrim()) {
      set_error("layer has no defaultPrim");
      return FREEUSD_ERR_NOT_FOUND;
    }
    const std::optional<std::string_view> dp = layer->inner->GetDefaultPrim();
    if (!dp.has_value()) {
      set_error("layer has no defaultPrim");
      return FREEUSD_ERR_NOT_FOUND;
    }
    const std::string s{*dp};
    *out_prim_name = dup_cstr(s);
    if (!*out_prim_name) {
      set_error("out of memory");
      return FREEUSD_ERR_INTERNAL;
    }
    clear_error();
    return FREEUSD_OK;
  } catch (const std::exception& e) {
    set_error(e.what());
    return FREEUSD_ERR_INTERNAL;
  } catch (...) {
    set_error("unknown exception");
    return FREEUSD_ERR_INTERNAL;
  }
}

int freeusd_layer_list_sub_layers(const FreeusdLayer* layer, char*** out_paths, size_t* out_count) {
  if (!layer || !layer->inner || !out_paths || !out_count) {
    set_error("freeusd_layer_list_sub_layers: null argument");
    return FREEUSD_ERR_INVALID_ARGUMENT;
  }
  *out_paths = nullptr;
  *out_count = 0;
  try {
    const std::vector<std::string>& subs = layer->inner->GetSubLayers();
    if (subs.empty()) {
      clear_error();
      return FREEUSD_OK;
    }
    char** arr = static_cast<char**>(std::malloc(subs.size() * sizeof(char*)));
    if (!arr) {
      set_error("out of memory");
      return FREEUSD_ERR_INTERNAL;
    }
    for (std::size_t i = 0; i < subs.size(); ++i) {
      arr[i] = dup_cstr(subs[i]);
      if (!arr[i]) {
        for (std::size_t j = 0; j < i; ++j) {
          std::free(arr[j]);
        }
        std::free(arr);
        set_error("out of memory");
        return FREEUSD_ERR_INTERNAL;
      }
    }
    *out_paths = arr;
    *out_count = subs.size();
    clear_error();
    return FREEUSD_OK;
  } catch (const std::exception& e) {
    set_error(e.what());
    return FREEUSD_ERR_INTERNAL;
  } catch (...) {
    set_error("unknown exception");
    return FREEUSD_ERR_INTERNAL;
  }
}

FreeusdLayerStack* freeusd_layer_stack_new(void) {
  try {
    clear_error();
    return new FreeusdLayerStack{};
  } catch (const std::exception& e) {
    set_error(e.what());
    return nullptr;
  } catch (...) {
    set_error("unknown exception");
    return nullptr;
  }
}

void freeusd_layer_stack_free(FreeusdLayerStack* stack) {
  clear_error();
  delete stack;
}

void freeusd_layer_stack_clear(FreeusdLayerStack* stack) {
  if (!stack) {
    set_error("freeusd_layer_stack_clear: null stack");
    return;
  }
  clear_error();
  stack->inner.Clear();
}

void freeusd_layer_stack_append(FreeusdLayerStack* stack, const FreeusdLayer* layer) {
  if (!stack || !layer || !layer->inner) {
    set_error("freeusd_layer_stack_append: null argument");
    return;
  }
  clear_error();
  stack->inner.Append(layer->inner);
}

int freeusd_stage_compose_layer_count(const FreeusdStage* stage, size_t* out_count) {
  if (!stage || !stage->inner || !out_count) {
    set_error("freeusd_stage_compose_layer_count: null argument");
    return FREEUSD_ERR_INVALID_ARGUMENT;
  }
  try {
    *out_count = stage->inner->GetComposeLayers().size();
    clear_error();
    return FREEUSD_OK;
  } catch (const std::exception& e) {
    set_error(e.what());
    return FREEUSD_ERR_INTERNAL;
  } catch (...) {
    set_error("unknown exception");
    return FREEUSD_ERR_INTERNAL;
  }
}

FreeusdStage* freeusd_stage_attach_layer_stack(const FreeusdLayerStack* stack) {
  if (!stack) {
    set_error("freeusd_stage_attach_layer_stack: null stack");
    return nullptr;
  }
  try {
    if (stack->inner.IsEmpty()) {
      set_error("layer stack is empty");
      return nullptr;
    }
    std::shared_ptr<freeusd::usd::Stage> st = freeusd::usd::Stage::AttachLayerStack(stack->inner);
    if (!st) {
      set_error("AttachLayerStack returned null");
      return nullptr;
    }
    clear_error();
    return new FreeusdStage{std::move(st)};
  } catch (const std::exception& e) {
    set_error(e.what());
    return nullptr;
  } catch (...) {
    set_error("unknown exception");
    return nullptr;
  }
}

FreeusdStage* freeusd_stage_attach_root_layer(const FreeusdLayer* layer) {
  if (!layer || !layer->inner) {
    set_error("freeusd_stage_attach_root_layer: null layer");
    return nullptr;
  }
  try {
    std::shared_ptr<freeusd::usd::Stage> st = freeusd::usd::Stage::AttachRootLayer(layer->inner);
    if (!st) {
      set_error("AttachRootLayer returned null");
      return nullptr;
    }
    clear_error();
    return new FreeusdStage{std::move(st)};
  } catch (const std::exception& e) {
    set_error(e.what());
    return nullptr;
  } catch (...) {
    set_error("unknown exception");
    return nullptr;
  }
}

FreeusdStage* freeusd_stage_open_from_root_file_utf8(const char* layer_path_utf8, int sublayer_policy) {
  clear_error();
  if (!layer_path_utf8) {
    set_error("freeusd_stage_open_from_root_file_utf8: null path");
    return nullptr;
  }
  freeusd::usd::RootLayerSublayersPolicy pol = freeusd::usd::RootLayerSublayersPolicy::DepthFirst;
  if (sublayer_policy == 0) {
    pol = freeusd::usd::RootLayerSublayersPolicy::None;
  } else if (sublayer_policy == 1) {
    pol = freeusd::usd::RootLayerSublayersPolicy::Shallow;
  } else if (sublayer_policy == 2) {
    pol = freeusd::usd::RootLayerSublayersPolicy::DepthFirst;
  } else {
    set_error("invalid sublayer_policy (use 0=none, 1=shallow, 2=depth_first)");
    return nullptr;
  }
  try {
    std::string err;
    std::shared_ptr<freeusd::usd::Stage> st =
        freeusd::usd::Stage::OpenFromRootFile(std::string{layer_path_utf8}, pol, &err);
    if (!st) {
      set_error(err.empty() ? "OpenFromRootFile failed" : err);
      return nullptr;
    }
    clear_error();
    return new FreeusdStage{std::move(st)};
  } catch (const std::exception& e) {
    set_error(e.what());
    return nullptr;
  } catch (...) {
    set_error("unknown exception");
    return nullptr;
  }
}

void freeusd_stage_free(FreeusdStage* stage) {
  clear_error();
  delete stage;
}

char* freeusd_stage_get_pseudo_root_path_utf8(const FreeusdStage* stage) {
  if (!stage || !stage->inner) {
    set_error("freeusd_stage_get_pseudo_root_path_utf8: null stage");
    return nullptr;
  }
  try {
    const std::string s = stage->inner->GetPseudoRootPath().GetString();
    char* out = dup_cstr(s);
    if (!out) {
      set_error("out of memory");
      return nullptr;
    }
    clear_error();
    return out;
  } catch (const std::exception& e) {
    set_error(e.what());
    return nullptr;
  } catch (...) {
    set_error("unknown exception");
    return nullptr;
  }
}

int freeusd_stage_get_start_time_code(const FreeusdStage* stage, double* out_value, int* out_has) {
  if (!stage || !stage->inner || !out_value || !out_has) {
    set_error("freeusd_stage_get_start_time_code: null argument");
    return FREEUSD_ERR_INVALID_ARGUMENT;
  }
  try {
    const std::optional<double> v = stage->inner->GetStartTimeCode();
    *out_has = v.has_value() ? 1 : 0;
    if (v.has_value()) {
      *out_value = *v;
    }
    clear_error();
    return FREEUSD_OK;
  } catch (const std::exception& e) {
    set_error(e.what());
    return FREEUSD_ERR_INTERNAL;
  } catch (...) {
    set_error("unknown exception");
    return FREEUSD_ERR_INTERNAL;
  }
}

int freeusd_stage_get_end_time_code(const FreeusdStage* stage, double* out_value, int* out_has) {
  if (!stage || !stage->inner || !out_value || !out_has) {
    set_error("freeusd_stage_get_end_time_code: null argument");
    return FREEUSD_ERR_INVALID_ARGUMENT;
  }
  try {
    const std::optional<double> v = stage->inner->GetEndTimeCode();
    *out_has = v.has_value() ? 1 : 0;
    if (v.has_value()) {
      *out_value = *v;
    }
    clear_error();
    return FREEUSD_OK;
  } catch (const std::exception& e) {
    set_error(e.what());
    return FREEUSD_ERR_INTERNAL;
  } catch (...) {
    set_error("unknown exception");
    return FREEUSD_ERR_INTERNAL;
  }
}

int freeusd_stage_get_time_codes_per_second(const FreeusdStage* stage, double* out_value, int* out_has) {
  if (!stage || !stage->inner || !out_value || !out_has) {
    set_error("freeusd_stage_get_time_codes_per_second: null argument");
    return FREEUSD_ERR_INVALID_ARGUMENT;
  }
  try {
    const std::optional<double> v = stage->inner->GetTimeCodesPerSecond();
    *out_has = v.has_value() ? 1 : 0;
    if (v.has_value()) {
      *out_value = *v;
    }
    clear_error();
    return FREEUSD_OK;
  } catch (const std::exception& e) {
    set_error(e.what());
    return FREEUSD_ERR_INTERNAL;
  } catch (...) {
    set_error("unknown exception");
    return FREEUSD_ERR_INTERNAL;
  }
}

int freeusd_stage_get_frames_per_second(const FreeusdStage* stage, double* out_value, int* out_has) {
  if (!stage || !stage->inner || !out_value || !out_has) {
    set_error("freeusd_stage_get_frames_per_second: null argument");
    return FREEUSD_ERR_INVALID_ARGUMENT;
  }
  try {
    const std::optional<double> v = stage->inner->GetFramesPerSecond();
    *out_has = v.has_value() ? 1 : 0;
    if (v.has_value()) {
      *out_value = *v;
    }
    clear_error();
    return FREEUSD_OK;
  } catch (const std::exception& e) {
    set_error(e.what());
    return FREEUSD_ERR_INTERNAL;
  } catch (...) {
    set_error("unknown exception");
    return FREEUSD_ERR_INTERNAL;
  }
}

int freeusd_stage_get_frame_precision(const FreeusdStage* stage, std::int64_t* out_value, int* out_has) {
  if (!stage || !stage->inner || !out_value || !out_has) {
    set_error("freeusd_stage_get_frame_precision: null argument");
    return FREEUSD_ERR_INVALID_ARGUMENT;
  }
  try {
    const std::optional<int> v = stage->inner->GetFramePrecision();
    *out_has = v.has_value() ? 1 : 0;
    if (v.has_value()) {
      *out_value = static_cast<std::int64_t>(*v);
    }
    clear_error();
    return FREEUSD_OK;
  } catch (const std::exception& e) {
    set_error(e.what());
    return FREEUSD_ERR_INTERNAL;
  } catch (...) {
    set_error("unknown exception");
    return FREEUSD_ERR_INTERNAL;
  }
}

int freeusd_stage_get_meters_per_unit(const FreeusdStage* stage, double* out_value, int* out_has) {
  if (!stage || !stage->inner || !out_value || !out_has) {
    set_error("freeusd_stage_get_meters_per_unit: null argument");
    return FREEUSD_ERR_INVALID_ARGUMENT;
  }
  try {
    const std::optional<double> v = stage->inner->GetMetersPerUnit();
    *out_has = v.has_value() ? 1 : 0;
    if (v.has_value()) {
      *out_value = *v;
    }
    clear_error();
    return FREEUSD_OK;
  } catch (const std::exception& e) {
    set_error(e.what());
    return FREEUSD_ERR_INTERNAL;
  } catch (...) {
    set_error("unknown exception");
    return FREEUSD_ERR_INTERNAL;
  }
}

int freeusd_stage_get_up_axis_utf8(const FreeusdStage* stage, char** out_axis_utf8) {
  if (!stage || !stage->inner || !out_axis_utf8) {
    set_error("freeusd_stage_get_up_axis_utf8: null argument");
    return FREEUSD_ERR_INVALID_ARGUMENT;
  }
  *out_axis_utf8 = nullptr;
  try {
    const std::optional<std::string> v = stage->inner->GetUpAxis();
    if (!v.has_value()) {
      set_error("no composed upAxis");
      return FREEUSD_ERR_NOT_FOUND;
    }
    char* dup = dup_cstr(*v);
    if (!dup) {
      set_error("out of memory");
      return FREEUSD_ERR_INTERNAL;
    }
    *out_axis_utf8 = dup;
    clear_error();
    return FREEUSD_OK;
  } catch (const std::exception& e) {
    set_error(e.what());
    return FREEUSD_ERR_INTERNAL;
  } catch (...) {
    set_error("unknown exception");
    return FREEUSD_ERR_INTERNAL;
  }
}

int freeusd_stage_list_prim_order_paths_utf8(const FreeusdStage* stage, char*** out_paths, size_t* out_count) {
  if (!stage || !stage->inner || !out_paths || !out_count) {
    set_error("freeusd_stage_list_prim_order_paths_utf8: null argument");
    return FREEUSD_ERR_INVALID_ARGUMENT;
  }
  *out_paths = nullptr;
  *out_count = 0;
  try {
    const std::vector<freeusd::sdf::Path> paths = stage->inner->GetPrimOrder();
    if (paths.empty()) {
      clear_error();
      return FREEUSD_OK;
    }
    std::vector<std::string> items;
    items.reserve(paths.size());
    for (const auto& p : paths) {
      items.push_back(p.GetString());
    }
    const int rc = malloc_string_list(items, out_paths, out_count);
    if (rc != FREEUSD_OK) {
      set_error(rc == FREEUSD_ERR_INTERNAL ? "out of memory" : "invalid argument");
      return rc;
    }
    clear_error();
    return FREEUSD_OK;
  } catch (const std::exception& e) {
    set_error(e.what());
    return FREEUSD_ERR_INTERNAL;
  } catch (...) {
    set_error("unknown exception");
    return FREEUSD_ERR_INTERNAL;
  }
}

int freeusd_stage_list_composed_prim_paths(const FreeusdStage* stage, char*** out_paths, size_t* out_count) {
  if (!stage || !stage->inner || !out_paths || !out_count) {
    set_error("freeusd_stage_list_composed_prim_paths: null argument");
    return FREEUSD_ERR_INVALID_ARGUMENT;
  }
  *out_paths = nullptr;
  *out_count = 0;
  try {
    const std::vector<freeusd::sdf::Path> paths = stage->inner->ListComposedPrimPaths();
    if (paths.empty()) {
      clear_error();
      return FREEUSD_OK;
    }
    char** arr = static_cast<char**>(std::malloc(paths.size() * sizeof(char*)));
    if (!arr) {
      set_error("out of memory");
      return FREEUSD_ERR_INTERNAL;
    }
    for (std::size_t i = 0; i < paths.size(); ++i) {
      arr[i] = dup_cstr(paths[i].GetString());
      if (!arr[i]) {
        for (std::size_t j = 0; j < i; ++j) {
          std::free(arr[j]);
        }
        std::free(arr);
        set_error("out of memory");
        return FREEUSD_ERR_INTERNAL;
      }
    }
    *out_paths = arr;
    *out_count = paths.size();
    clear_error();
    return FREEUSD_OK;
  } catch (const std::exception& e) {
    set_error(e.what());
    return FREEUSD_ERR_INTERNAL;
  } catch (...) {
    set_error("unknown exception");
    return FREEUSD_ERR_INTERNAL;
  }
}

int freeusd_stage_has_default_prim(const FreeusdStage* stage) {
  if (!stage || !stage->inner) {
    set_error("freeusd_stage_has_default_prim: null stage");
    return FREEUSD_ERR_INVALID_ARGUMENT;
  }
  try {
    const int out = stage->inner->HasDefaultPrim() ? 1 : 0;
    clear_error();
    return out;
  } catch (const std::exception& e) {
    set_error(e.what());
    return FREEUSD_ERR_INTERNAL;
  } catch (...) {
    set_error("unknown exception");
    return FREEUSD_ERR_INTERNAL;
  }
}

int freeusd_stage_get_default_prim_utf8(const FreeusdStage* stage, char** out_prim_name) {
  if (!stage || !stage->inner || !out_prim_name) {
    set_error("freeusd_stage_get_default_prim_utf8: null argument");
    return FREEUSD_ERR_INVALID_ARGUMENT;
  }
  *out_prim_name = nullptr;
  try {
    if (!stage->inner->HasDefaultPrim()) {
      set_error("stage root layer has no defaultPrim");
      return FREEUSD_ERR_NOT_FOUND;
    }
    const std::string s = stage->inner->GetDefaultPrimName();
    if (s.empty()) {
      set_error("stage root layer has no defaultPrim");
      return FREEUSD_ERR_NOT_FOUND;
    }
    *out_prim_name = dup_cstr(s);
    if (!*out_prim_name) {
      set_error("out of memory");
      return FREEUSD_ERR_INTERNAL;
    }
    clear_error();
    return FREEUSD_OK;
  } catch (const std::exception& e) {
    set_error(e.what());
    return FREEUSD_ERR_INTERNAL;
  } catch (...) {
    set_error("unknown exception");
    return FREEUSD_ERR_INTERNAL;
  }
}

int freeusd_stage_list_child_paths(const FreeusdStage* stage, const char* parent_prim_utf8, char*** out_paths,
                                   size_t* out_count) {
  if (!stage || !stage->inner || !parent_prim_utf8 || !out_paths || !out_count) {
    set_error("freeusd_stage_list_child_paths: null argument");
    return FREEUSD_ERR_INVALID_ARGUMENT;
  }
  *out_paths = nullptr;
  *out_count = 0;
  try {
    const freeusd::sdf::Path parent = freeusd::sdf::Path::FromString(parent_prim_utf8);
    if (parent.IsEmpty()) {
      set_error("invalid parent prim path");
      return FREEUSD_ERR_INVALID_ARGUMENT;
    }
    if (!parent.IsPrimPath() && !parent.IsAbsoluteRootPath()) {
      set_error("parent path must be a prim path or /");
      return FREEUSD_ERR_INVALID_ARGUMENT;
    }
    std::vector<freeusd::usd::Prim> kids = stage->inner->GetChildren(parent);
    std::vector<std::string> paths;
    paths.reserve(kids.size());
    for (const freeusd::usd::Prim& pr : kids) {
      if (!pr.IsValid()) {
        continue;
      }
      paths.push_back(pr.GetPath().GetString());
    }
    std::sort(paths.begin(), paths.end());
    if (paths.empty()) {
      clear_error();
      return FREEUSD_OK;
    }
    char** arr = static_cast<char**>(std::malloc(paths.size() * sizeof(char*)));
    if (!arr) {
      set_error("out of memory");
      return FREEUSD_ERR_INTERNAL;
    }
    for (std::size_t i = 0; i < paths.size(); ++i) {
      arr[i] = dup_cstr(paths[i]);
      if (!arr[i]) {
        for (std::size_t j = 0; j < i; ++j) {
          std::free(arr[j]);
        }
        std::free(arr);
        set_error("out of memory");
        return FREEUSD_ERR_INTERNAL;
      }
    }
    *out_paths = arr;
    *out_count = paths.size();
    clear_error();
    return FREEUSD_OK;
  } catch (const std::exception& e) {
    set_error(e.what());
    return FREEUSD_ERR_INTERNAL;
  } catch (...) {
    set_error("unknown exception");
    return FREEUSD_ERR_INTERNAL;
  }
}

void freeusd_path_list_free(char** paths, size_t count) {
  if (!paths) {
    return;
  }
  for (size_t i = 0; i < count; ++i) {
    std::free(paths[i]);
  }
  std::free(paths);
}

void freeusd_double_array_free(double* values) { std::free(values); }

int freeusd_stage_list_relationship_targets(const FreeusdStage* stage, const char* prim_path_utf8,
                                            const char* rel_name_utf8, char*** out_paths, size_t* out_count) {
  if (!stage || !stage->inner || !prim_path_utf8 || !rel_name_utf8 || !out_paths || !out_count) {
    set_error("freeusd_stage_list_relationship_targets: null argument");
    return FREEUSD_ERR_INVALID_ARGUMENT;
  }
  *out_paths = nullptr;
  *out_count = 0;
  try {
    const freeusd::sdf::Path p = freeusd::sdf::Path::FromString(prim_path_utf8);
    if (p.IsEmpty()) {
      set_error("invalid prim path");
      return FREEUSD_ERR_INVALID_ARGUMENT;
    }
    std::vector<freeusd::sdf::Path> tgts;
    (void)stage->inner->ReadRelationship(p, freeusd::tf::Token{rel_name_utf8}, &tgts);
    if (tgts.empty()) {
      clear_error();
      return FREEUSD_OK;
    }
    char** arr = static_cast<char**>(std::malloc(tgts.size() * sizeof(char*)));
    if (!arr) {
      set_error("out of memory");
      return FREEUSD_ERR_INTERNAL;
    }
    for (std::size_t i = 0; i < tgts.size(); ++i) {
      arr[i] = dup_cstr(tgts[i].GetString());
      if (!arr[i]) {
        for (std::size_t j = 0; j < i; ++j) {
          std::free(arr[j]);
        }
        std::free(arr);
        set_error("out of memory");
        return FREEUSD_ERR_INTERNAL;
      }
    }
    *out_paths = arr;
    *out_count = tgts.size();
    clear_error();
    return FREEUSD_OK;
  } catch (const std::exception& e) {
    set_error(e.what());
    return FREEUSD_ERR_INTERNAL;
  } catch (...) {
    set_error("unknown exception");
    return FREEUSD_ERR_INTERNAL;
  }
}

int freeusd_stage_has_relationship(const FreeusdStage* stage, const char* prim_path_utf8,
                                   const char* rel_name_utf8) {
  if (!stage || !stage->inner || !prim_path_utf8 || !rel_name_utf8) {
    set_error("freeusd_stage_has_relationship: null argument");
    return FREEUSD_ERR_INVALID_ARGUMENT;
  }
  try {
    const freeusd::sdf::Path p = freeusd::sdf::Path::FromString(prim_path_utf8);
    if (p.IsEmpty()) {
      set_error("invalid prim path");
      return FREEUSD_ERR_INVALID_ARGUMENT;
    }
    const int out = stage->inner->HasRelationship(p, freeusd::tf::Token{rel_name_utf8}) ? 1 : 0;
    clear_error();
    return out;
  } catch (const std::exception& e) {
    set_error(e.what());
    return FREEUSD_ERR_INTERNAL;
  } catch (...) {
    set_error("unknown exception");
    return FREEUSD_ERR_INTERNAL;
  }
}

int freeusd_stage_relocate_source_in_any_layer(const FreeusdStage* stage, const char* from_prim_utf8) {
  if (!stage || !stage->inner || !from_prim_utf8) {
    set_error("freeusd_stage_relocate_source_in_any_layer: null argument");
    return FREEUSD_ERR_INVALID_ARGUMENT;
  }
  try {
    const freeusd::sdf::Path p = freeusd::sdf::Path::FromString(from_prim_utf8);
    if (p.IsEmpty() || !p.IsPrimPath()) {
      set_error("invalid prim path");
      return FREEUSD_ERR_INVALID_ARGUMENT;
    }
    const int out = stage->inner->RelocateSourceInAnyLayer(p) ? 1 : 0;
    clear_error();
    return out;
  } catch (const std::exception& e) {
    set_error(e.what());
    return FREEUSD_ERR_INTERNAL;
  } catch (...) {
    set_error("unknown exception");
    return FREEUSD_ERR_INTERNAL;
  }
}

int freeusd_stage_get_composed_relocate_target_utf8(const FreeusdStage* stage, const char* from_prim_utf8,
                                                    char** out_target_utf8) {
  if (!stage || !stage->inner || !from_prim_utf8 || !out_target_utf8) {
    set_error("freeusd_stage_get_composed_relocate_target_utf8: null argument");
    return FREEUSD_ERR_INVALID_ARGUMENT;
  }
  *out_target_utf8 = nullptr;
  try {
    const freeusd::sdf::Path p = freeusd::sdf::Path::FromString(from_prim_utf8);
    if (p.IsEmpty() || !p.IsPrimPath()) {
      set_error("invalid prim path");
      return FREEUSD_ERR_INVALID_ARGUMENT;
    }
    freeusd::sdf::Path to;
    if (!stage->inner->GetComposedRelocateTarget(p, &to)) {
      set_error("no composed relocate for source");
      return FREEUSD_ERR_NOT_FOUND;
    }
    char* dup = dup_cstr(to.GetString());
    if (!dup) {
      set_error("out of memory");
      return FREEUSD_ERR_INTERNAL;
    }
    *out_target_utf8 = dup;
    clear_error();
    return FREEUSD_OK;
  } catch (const std::exception& e) {
    set_error(e.what());
    return FREEUSD_ERR_INTERNAL;
  } catch (...) {
    set_error("unknown exception");
    return FREEUSD_ERR_INTERNAL;
  }
}

int freeusd_stage_list_composed_relocate_pairs_utf8(const FreeusdStage* stage, char*** out_strings, size_t* out_count) {
  if (!stage || !stage->inner || !out_strings || !out_count) {
    set_error("freeusd_stage_list_composed_relocate_pairs_utf8: null argument");
    return FREEUSD_ERR_INVALID_ARGUMENT;
  }
  *out_strings = nullptr;
  *out_count = 0;
  try {
    constexpr char kSep = '\x1f';
    const std::vector<std::pair<freeusd::sdf::Path, freeusd::sdf::Path>> pairs = stage->inner->ListComposedRelocates();
    std::vector<std::string> items;
    items.reserve(pairs.size());
    for (const auto& pr : pairs) {
      items.push_back(pr.first.GetString() + kSep + pr.second.GetString());
    }
    const int rc = malloc_string_list(items, out_strings, out_count);
    if (rc != FREEUSD_OK) {
      set_error(rc == FREEUSD_ERR_INTERNAL ? "out of memory" : "invalid argument");
      return rc;
    }
    clear_error();
    return FREEUSD_OK;
  } catch (const std::exception& e) {
    set_error(e.what());
    return FREEUSD_ERR_INTERNAL;
  } catch (...) {
    set_error("unknown exception");
    return FREEUSD_ERR_INTERNAL;
  }
}

int freeusd_stage_prefix_substitution_key_in_any_layer(const FreeusdStage* stage, const char* from_prefix_utf8) {
  if (!stage || !stage->inner || !from_prefix_utf8) {
    set_error("freeusd_stage_prefix_substitution_key_in_any_layer: null argument");
    return FREEUSD_ERR_INVALID_ARGUMENT;
  }
  try {
    const int out = stage->inner->PrefixSubstitutionKeyInAnyLayer(std::string{from_prefix_utf8}) ? 1 : 0;
    clear_error();
    return out;
  } catch (const std::exception& e) {
    set_error(e.what());
    return FREEUSD_ERR_INTERNAL;
  } catch (...) {
    set_error("unknown exception");
    return FREEUSD_ERR_INTERNAL;
  }
}

int freeusd_stage_get_composed_prefix_substitution_utf8(const FreeusdStage* stage, const char* from_prefix_utf8,
                                                        char** out_to_prefix_utf8) {
  if (!stage || !stage->inner || !from_prefix_utf8 || !out_to_prefix_utf8) {
    set_error("freeusd_stage_get_composed_prefix_substitution_utf8: null argument");
    return FREEUSD_ERR_INVALID_ARGUMENT;
  }
  *out_to_prefix_utf8 = nullptr;
  try {
    const std::string from{from_prefix_utf8};
    if (from.empty()) {
      set_error("empty from_prefix");
      return FREEUSD_ERR_INVALID_ARGUMENT;
    }
    std::string to;
    if (!stage->inner->GetComposedPrefixSubstitution(from, &to)) {
      set_error("no composed prefix substitution for key");
      return FREEUSD_ERR_NOT_FOUND;
    }
    char* dup = dup_cstr(to);
    if (!dup) {
      set_error("out of memory");
      return FREEUSD_ERR_INTERNAL;
    }
    *out_to_prefix_utf8 = dup;
    clear_error();
    return FREEUSD_OK;
  } catch (const std::exception& e) {
    set_error(e.what());
    return FREEUSD_ERR_INTERNAL;
  } catch (...) {
    set_error("unknown exception");
    return FREEUSD_ERR_INTERNAL;
  }
}

int freeusd_stage_list_composed_prefix_substitution_pairs_utf8(const FreeusdStage* stage, char*** out_strings,
                                                               size_t* out_count) {
  if (!stage || !stage->inner || !out_strings || !out_count) {
    set_error("freeusd_stage_list_composed_prefix_substitution_pairs_utf8: null argument");
    return FREEUSD_ERR_INVALID_ARGUMENT;
  }
  *out_strings = nullptr;
  *out_count = 0;
  try {
    constexpr char kSep = '\x1f';
    const std::vector<std::pair<std::string, std::string>> pairs = stage->inner->ListComposedPrefixSubstitutions();
    std::vector<std::string> items;
    items.reserve(pairs.size());
    for (const auto& pr : pairs) {
      items.push_back(pr.first + kSep + pr.second);
    }
    const int rc = malloc_string_list(items, out_strings, out_count);
    if (rc != FREEUSD_OK) {
      set_error(rc == FREEUSD_ERR_INTERNAL ? "out of memory" : "invalid argument");
      return rc;
    }
    clear_error();
    return FREEUSD_OK;
  } catch (const std::exception& e) {
    set_error(e.what());
    return FREEUSD_ERR_INTERNAL;
  } catch (...) {
    set_error("unknown exception");
    return FREEUSD_ERR_INTERNAL;
  }
}

int freeusd_stage_list_prim_references(const FreeusdStage* stage, const char* prim_path_utf8, char*** out_strings,
                                       size_t* out_count) {
  if (!stage || !stage->inner || !prim_path_utf8 || !out_strings || !out_count) {
    set_error("freeusd_stage_list_prim_references: null argument");
    return FREEUSD_ERR_INVALID_ARGUMENT;
  }
  *out_strings = nullptr;
  *out_count = 0;
  try {
    const freeusd::sdf::Path p = freeusd::sdf::Path::FromString(prim_path_utf8);
    if (p.IsEmpty()) {
      set_error("invalid prim path");
      return FREEUSD_ERR_INVALID_ARGUMENT;
    }
    std::vector<std::string> items;
    for (const freeusd::sdf::PrimReference& r : stage->inner->ReadPrimReferences(p)) {
      items.push_back(r.FormatAuthoredForUsda());
    }
    const int rc = malloc_string_list(items, out_strings, out_count);
    if (rc != FREEUSD_OK) {
      set_error(rc == FREEUSD_ERR_INTERNAL ? "out of memory" : "invalid argument");
      return rc;
    }
    clear_error();
    return FREEUSD_OK;
  } catch (const std::exception& e) {
    set_error(e.what());
    return FREEUSD_ERR_INTERNAL;
  } catch (...) {
    set_error("unknown exception");
    return FREEUSD_ERR_INTERNAL;
  }
}

int freeusd_stage_has_prim_references(const FreeusdStage* stage, const char* prim_path_utf8) {
  if (!stage || !stage->inner || !prim_path_utf8) {
    set_error("freeusd_stage_has_prim_references: null argument");
    return FREEUSD_ERR_INVALID_ARGUMENT;
  }
  try {
    const freeusd::sdf::Path p = freeusd::sdf::Path::FromString(prim_path_utf8);
    if (p.IsEmpty()) {
      set_error("invalid prim path");
      return FREEUSD_ERR_INVALID_ARGUMENT;
    }
    const int out = stage->inner->HasPrimReferences(p) ? 1 : 0;
    clear_error();
    return out;
  } catch (const std::exception& e) {
    set_error(e.what());
    return FREEUSD_ERR_INTERNAL;
  } catch (...) {
    set_error("unknown exception");
    return FREEUSD_ERR_INTERNAL;
  }
}

int freeusd_stage_list_prim_inherits(const FreeusdStage* stage, const char* prim_path_utf8, char*** out_paths,
                                     size_t* out_count) {
  if (!stage || !stage->inner || !prim_path_utf8 || !out_paths || !out_count) {
    set_error("freeusd_stage_list_prim_inherits: null argument");
    return FREEUSD_ERR_INVALID_ARGUMENT;
  }
  *out_paths = nullptr;
  *out_count = 0;
  try {
    const freeusd::sdf::Path p = freeusd::sdf::Path::FromString(prim_path_utf8);
    if (p.IsEmpty()) {
      set_error("invalid prim path");
      return FREEUSD_ERR_INVALID_ARGUMENT;
    }
    std::vector<std::string> items;
    for (const freeusd::sdf::Path& ip : stage->inner->ReadPrimInherits(p)) {
      items.push_back(ip.GetString());
    }
    const int rc = malloc_string_list(items, out_paths, out_count);
    if (rc != FREEUSD_OK) {
      set_error(rc == FREEUSD_ERR_INTERNAL ? "out of memory" : "invalid argument");
      return rc;
    }
    clear_error();
    return FREEUSD_OK;
  } catch (const std::exception& e) {
    set_error(e.what());
    return FREEUSD_ERR_INTERNAL;
  } catch (...) {
    set_error("unknown exception");
    return FREEUSD_ERR_INTERNAL;
  }
}

int freeusd_stage_has_prim_inherits(const FreeusdStage* stage, const char* prim_path_utf8) {
  if (!stage || !stage->inner || !prim_path_utf8) {
    set_error("freeusd_stage_has_prim_inherits: null argument");
    return FREEUSD_ERR_INVALID_ARGUMENT;
  }
  try {
    const freeusd::sdf::Path p = freeusd::sdf::Path::FromString(prim_path_utf8);
    if (p.IsEmpty()) {
      set_error("invalid prim path");
      return FREEUSD_ERR_INVALID_ARGUMENT;
    }
    const int out = stage->inner->HasPrimInherits(p) ? 1 : 0;
    clear_error();
    return out;
  } catch (const std::exception& e) {
    set_error(e.what());
    return FREEUSD_ERR_INTERNAL;
  } catch (...) {
    set_error("unknown exception");
    return FREEUSD_ERR_INTERNAL;
  }
}

int freeusd_stage_list_prim_specializes(const FreeusdStage* stage, const char* prim_path_utf8, char*** out_paths,
                                        size_t* out_count) {
  if (!stage || !stage->inner || !prim_path_utf8 || !out_paths || !out_count) {
    set_error("freeusd_stage_list_prim_specializes: null argument");
    return FREEUSD_ERR_INVALID_ARGUMENT;
  }
  *out_paths = nullptr;
  *out_count = 0;
  try {
    const freeusd::sdf::Path p = freeusd::sdf::Path::FromString(prim_path_utf8);
    if (p.IsEmpty()) {
      set_error("invalid prim path");
      return FREEUSD_ERR_INVALID_ARGUMENT;
    }
    std::vector<std::string> items;
    for (const freeusd::sdf::Path& sp : stage->inner->ReadPrimSpecializes(p)) {
      items.push_back(sp.GetString());
    }
    const int rc = malloc_string_list(items, out_paths, out_count);
    if (rc != FREEUSD_OK) {
      set_error(rc == FREEUSD_ERR_INTERNAL ? "out of memory" : "invalid argument");
      return rc;
    }
    clear_error();
    return FREEUSD_OK;
  } catch (const std::exception& e) {
    set_error(e.what());
    return FREEUSD_ERR_INTERNAL;
  } catch (...) {
    set_error("unknown exception");
    return FREEUSD_ERR_INTERNAL;
  }
}

int freeusd_stage_has_prim_specializes(const FreeusdStage* stage, const char* prim_path_utf8) {
  if (!stage || !stage->inner || !prim_path_utf8) {
    set_error("freeusd_stage_has_prim_specializes: null argument");
    return FREEUSD_ERR_INVALID_ARGUMENT;
  }
  try {
    const freeusd::sdf::Path p = freeusd::sdf::Path::FromString(prim_path_utf8);
    if (p.IsEmpty()) {
      set_error("invalid prim path");
      return FREEUSD_ERR_INVALID_ARGUMENT;
    }
    const int out = stage->inner->HasPrimSpecializes(p) ? 1 : 0;
    clear_error();
    return out;
  } catch (const std::exception& e) {
    set_error(e.what());
    return FREEUSD_ERR_INTERNAL;
  } catch (...) {
    set_error("unknown exception");
    return FREEUSD_ERR_INTERNAL;
  }
}

int freeusd_stage_list_prim_payloads(const FreeusdStage* stage, const char* prim_path_utf8, char*** out_strings,
                                     size_t* out_count) {
  if (!stage || !stage->inner || !prim_path_utf8 || !out_strings || !out_count) {
    set_error("freeusd_stage_list_prim_payloads: null argument");
    return FREEUSD_ERR_INVALID_ARGUMENT;
  }
  *out_strings = nullptr;
  *out_count = 0;
  try {
    const freeusd::sdf::Path p = freeusd::sdf::Path::FromString(prim_path_utf8);
    if (p.IsEmpty()) {
      set_error("invalid prim path");
      return FREEUSD_ERR_INVALID_ARGUMENT;
    }
    std::vector<std::string> items;
    for (const freeusd::sdf::PrimReference& r : stage->inner->ReadPrimPayloads(p)) {
      items.push_back(r.FormatAuthoredForUsda());
    }
    const int rc = malloc_string_list(items, out_strings, out_count);
    if (rc != FREEUSD_OK) {
      set_error(rc == FREEUSD_ERR_INTERNAL ? "out of memory" : "invalid argument");
      return rc;
    }
    clear_error();
    return FREEUSD_OK;
  } catch (const std::exception& e) {
    set_error(e.what());
    return FREEUSD_ERR_INTERNAL;
  } catch (...) {
    set_error("unknown exception");
    return FREEUSD_ERR_INTERNAL;
  }
}

int freeusd_stage_has_prim_payloads(const FreeusdStage* stage, const char* prim_path_utf8) {
  if (!stage || !stage->inner || !prim_path_utf8) {
    set_error("freeusd_stage_has_prim_payloads: null argument");
    return FREEUSD_ERR_INVALID_ARGUMENT;
  }
  try {
    const freeusd::sdf::Path p = freeusd::sdf::Path::FromString(prim_path_utf8);
    if (p.IsEmpty()) {
      set_error("invalid prim path");
      return FREEUSD_ERR_INVALID_ARGUMENT;
    }
    const int out = stage->inner->HasPrimPayloads(p) ? 1 : 0;
    clear_error();
    return out;
  } catch (const std::exception& e) {
    set_error(e.what());
    return FREEUSD_ERR_INTERNAL;
  } catch (...) {
    set_error("unknown exception");
    return FREEUSD_ERR_INTERNAL;
  }
}

int freeusd_stage_list_composed_field_names(const FreeusdStage* stage, const char* prim_path_utf8,
                                            char*** out_names, size_t* out_count) {
  if (!stage || !stage->inner || !prim_path_utf8 || !out_names || !out_count) {
    set_error("freeusd_stage_list_composed_field_names: null argument");
    return FREEUSD_ERR_INVALID_ARGUMENT;
  }
  *out_names = nullptr;
  *out_count = 0;
  try {
    const freeusd::sdf::Path p = freeusd::sdf::Path::FromString(prim_path_utf8);
    if (p.IsEmpty()) {
      set_error("invalid prim path");
      return FREEUSD_ERR_INVALID_ARGUMENT;
    }
    const std::vector<std::string> keys = stage->inner->ListComposedFieldNames(p);
    if (keys.empty()) {
      clear_error();
      return FREEUSD_OK;
    }
    char** arr = static_cast<char**>(std::malloc(keys.size() * sizeof(char*)));
    if (!arr) {
      set_error("out of memory");
      return FREEUSD_ERR_INTERNAL;
    }
    for (std::size_t i = 0; i < keys.size(); ++i) {
      arr[i] = dup_cstr(keys[i]);
      if (!arr[i]) {
        for (std::size_t j = 0; j < i; ++j) {
          std::free(arr[j]);
        }
        std::free(arr);
        set_error("out of memory");
        return FREEUSD_ERR_INTERNAL;
      }
    }
    *out_names = arr;
    *out_count = keys.size();
    clear_error();
    return FREEUSD_OK;
  } catch (const std::exception& e) {
    set_error(e.what());
    return FREEUSD_ERR_INTERNAL;
  } catch (...) {
    set_error("unknown exception");
    return FREEUSD_ERR_INTERNAL;
  }
}

int freeusd_stage_list_composed_relationship_names(const FreeusdStage* stage, const char* prim_path_utf8,
                                                   char*** out_names, size_t* out_count) {
  if (!stage || !stage->inner || !prim_path_utf8 || !out_names || !out_count) {
    set_error("freeusd_stage_list_composed_relationship_names: null argument");
    return FREEUSD_ERR_INVALID_ARGUMENT;
  }
  *out_names = nullptr;
  *out_count = 0;
  try {
    const freeusd::sdf::Path p = freeusd::sdf::Path::FromString(prim_path_utf8);
    if (p.IsEmpty()) {
      set_error("invalid prim path");
      return FREEUSD_ERR_INVALID_ARGUMENT;
    }
    const std::vector<std::string> keys = stage->inner->ListComposedRelationshipNames(p);
    if (keys.empty()) {
      clear_error();
      return FREEUSD_OK;
    }
    char** arr = static_cast<char**>(std::malloc(keys.size() * sizeof(char*)));
    if (!arr) {
      set_error("out of memory");
      return FREEUSD_ERR_INTERNAL;
    }
    for (std::size_t i = 0; i < keys.size(); ++i) {
      arr[i] = dup_cstr(keys[i]);
      if (!arr[i]) {
        for (std::size_t j = 0; j < i; ++j) {
          std::free(arr[j]);
        }
        std::free(arr);
        set_error("out of memory");
        return FREEUSD_ERR_INTERNAL;
      }
    }
    *out_names = arr;
    *out_count = keys.size();
    clear_error();
    return FREEUSD_OK;
  } catch (const std::exception& e) {
    set_error(e.what());
    return FREEUSD_ERR_INTERNAL;
  } catch (...) {
    set_error("unknown exception");
    return FREEUSD_ERR_INTERNAL;
  }
}

int freeusd_stage_prim_is_valid(const FreeusdStage* stage, const char* prim_path_utf8) {
  if (!stage || !stage->inner || !prim_path_utf8) {
    set_error("freeusd_stage_prim_is_valid: null argument");
    return FREEUSD_ERR_INVALID_ARGUMENT;
  }
  try {
    const freeusd::sdf::Path p = freeusd::sdf::Path::FromString(prim_path_utf8);
    if (p.IsEmpty()) {
      clear_error();
      return 0;
    }
    const bool ok = stage->inner->PrimPathInUse(p);
    clear_error();
    return ok ? 1 : 0;
  } catch (const std::exception& e) {
    set_error(e.what());
    return FREEUSD_ERR_INTERNAL;
  } catch (...) {
    set_error("unknown exception");
    return FREEUSD_ERR_INTERNAL;
  }
}

int freeusd_stage_compute_local_transform_matrix4d(const FreeusdStage* stage, const char* prim_path_utf8, double time,
                                                   double* out_row_major) {
  if (!stage || !stage->inner || !prim_path_utf8 || !out_row_major) {
    set_error("freeusd_stage_compute_local_transform_matrix4d: null argument");
    return FREEUSD_ERR_INVALID_ARGUMENT;
  }
  try {
    const freeusd::sdf::Path p = freeusd::sdf::Path::FromString(prim_path_utf8);
    if (p.IsEmpty()) {
      set_error("invalid prim path");
      return FREEUSD_ERR_INVALID_ARGUMENT;
    }
    const freeusd::usd::Prim prim = stage->inner->GetPrimAtPath(p);
    if (!prim.IsValid()) {
      set_error("prim not found");
      return FREEUSD_ERR_NOT_FOUND;
    }
    const freeusd::gf::Matrix4d m = freeusd::usdGeom::Xformable(prim).ComputeLocalTransform(time);
    std::memcpy(out_row_major, m.data(), 16u * sizeof(double));
    clear_error();
    return FREEUSD_OK;
  } catch (const std::exception& e) {
    set_error(e.what());
    return FREEUSD_ERR_INTERNAL;
  } catch (...) {
    set_error("unknown exception");
    return FREEUSD_ERR_INTERNAL;
  }
}

int freeusd_stage_compute_local_to_world_transform_matrix4d(const FreeusdStage* stage, const char* prim_path_utf8,
                                                            double time, double* out_row_major) {
  if (!stage || !stage->inner || !prim_path_utf8 || !out_row_major) {
    set_error("freeusd_stage_compute_local_to_world_transform_matrix4d: null argument");
    return FREEUSD_ERR_INVALID_ARGUMENT;
  }
  try {
    const freeusd::sdf::Path p = freeusd::sdf::Path::FromString(prim_path_utf8);
    if (p.IsEmpty()) {
      set_error("invalid prim path");
      return FREEUSD_ERR_INVALID_ARGUMENT;
    }
    const freeusd::usd::Prim prim = stage->inner->GetPrimAtPath(p);
    if (!prim.IsValid()) {
      set_error("prim not found");
      return FREEUSD_ERR_NOT_FOUND;
    }
    const freeusd::gf::Matrix4d m = freeusd::usdGeom::Xformable(prim).ComputeLocalToWorldTransform(time);
    std::memcpy(out_row_major, m.data(), 16u * sizeof(double));
    clear_error();
    return FREEUSD_OK;
  } catch (const std::exception& e) {
    set_error(e.what());
    return FREEUSD_ERR_INTERNAL;
  } catch (...) {
    set_error("unknown exception");
    return FREEUSD_ERR_INTERNAL;
  }
}

int freeusd_stage_compute_imageable_visibility(const FreeusdStage* stage, const char* prim_path_utf8, double time,
                                               int* out_visible) {
  if (!stage || !stage->inner || !prim_path_utf8 || !out_visible) {
    set_error("freeusd_stage_compute_imageable_visibility: null argument");
    return FREEUSD_ERR_INVALID_ARGUMENT;
  }
  try {
    const freeusd::sdf::Path p = freeusd::sdf::Path::FromString(prim_path_utf8);
    if (p.IsEmpty()) {
      set_error("invalid prim path");
      return FREEUSD_ERR_INVALID_ARGUMENT;
    }
    const freeusd::usd::Prim prim = stage->inner->GetPrimAtPath(p);
    if (!prim.IsValid()) {
      set_error("prim not found");
      return FREEUSD_ERR_NOT_FOUND;
    }
    *out_visible = freeusd::usdGeom::Imageable(prim).ComputeVisibility(time) ? 1 : 0;
    clear_error();
    return FREEUSD_OK;
  } catch (const std::exception& e) {
    set_error(e.what());
    return FREEUSD_ERR_INTERNAL;
  } catch (...) {
    set_error("unknown exception");
    return FREEUSD_ERR_INTERNAL;
  }
}

int freeusd_stage_compute_imageable_purpose_utf8(const FreeusdStage* stage, const char* prim_path_utf8, double time,
                                                 char** out_purpose_utf8) {
  if (!stage || !stage->inner || !prim_path_utf8 || !out_purpose_utf8) {
    set_error("freeusd_stage_compute_imageable_purpose_utf8: null argument");
    return FREEUSD_ERR_INVALID_ARGUMENT;
  }
  *out_purpose_utf8 = nullptr;
  try {
    const freeusd::sdf::Path p = freeusd::sdf::Path::FromString(prim_path_utf8);
    if (p.IsEmpty()) {
      set_error("invalid prim path");
      return FREEUSD_ERR_INVALID_ARGUMENT;
    }
    const freeusd::usd::Prim prim = stage->inner->GetPrimAtPath(p);
    if (!prim.IsValid()) {
      set_error("prim not found");
      return FREEUSD_ERR_NOT_FOUND;
    }
    const std::string purpose = freeusd::usdGeom::Imageable(prim).ComputePurpose(time);
    *out_purpose_utf8 = dup_cstr(purpose);
    if (!*out_purpose_utf8) {
      set_error("out of memory");
      return FREEUSD_ERR_INTERNAL;
    }
    clear_error();
    return FREEUSD_OK;
  } catch (const std::exception& e) {
    set_error(e.what());
    return FREEUSD_ERR_INTERNAL;
  } catch (...) {
    set_error("unknown exception");
    return FREEUSD_ERR_INTERNAL;
  }
}

int freeusd_stage_compute_boundable_local_bounds(const FreeusdStage* stage, const char* prim_path_utf8, double time,
                                                 double* out_min_x, double* out_min_y, double* out_min_z,
                                                 double* out_max_x, double* out_max_y, double* out_max_z) {
  if (!stage || !stage->inner || !prim_path_utf8 || !out_min_x || !out_min_y || !out_min_z || !out_max_x || !out_max_y ||
      !out_max_z) {
    set_error("freeusd_stage_compute_boundable_local_bounds: null argument");
    return FREEUSD_ERR_INVALID_ARGUMENT;
  }
  try {
    const freeusd::sdf::Path p = freeusd::sdf::Path::FromString(prim_path_utf8);
    if (p.IsEmpty()) {
      set_error("invalid prim path");
      return FREEUSD_ERR_INVALID_ARGUMENT;
    }
    const freeusd::usd::Prim prim = stage->inner->GetPrimAtPath(p);
    if (!prim.IsValid()) {
      set_error("prim not found");
      return FREEUSD_ERR_NOT_FOUND;
    }
    const freeusd::gf::BBox3d bounds = freeusd::usdGeom::Boundable(prim).ComputeLocalBound(time);
    if (bounds.IsEmpty()) {
      set_error("boundable has no local bounds");
      return FREEUSD_ERR_NOT_FOUND;
    }
    *out_min_x = bounds.min.x();
    *out_min_y = bounds.min.y();
    *out_min_z = bounds.min.z();
    *out_max_x = bounds.max.x();
    *out_max_y = bounds.max.y();
    *out_max_z = bounds.max.z();
    clear_error();
    return FREEUSD_OK;
  } catch (const std::exception& e) {
    set_error(e.what());
    return FREEUSD_ERR_INTERNAL;
  } catch (...) {
    set_error("unknown exception");
    return FREEUSD_ERR_INTERNAL;
  }
}

int freeusd_stage_compute_boundable_world_bounds(const FreeusdStage* stage, const char* prim_path_utf8, double time,
                                                 double* out_min_x, double* out_min_y, double* out_min_z,
                                                 double* out_max_x, double* out_max_y, double* out_max_z) {
  if (!stage || !stage->inner || !prim_path_utf8 || !out_min_x || !out_min_y || !out_min_z || !out_max_x || !out_max_y ||
      !out_max_z) {
    set_error("freeusd_stage_compute_boundable_world_bounds: null argument");
    return FREEUSD_ERR_INVALID_ARGUMENT;
  }
  try {
    const freeusd::sdf::Path p = freeusd::sdf::Path::FromString(prim_path_utf8);
    if (p.IsEmpty()) {
      set_error("invalid prim path");
      return FREEUSD_ERR_INVALID_ARGUMENT;
    }
    const freeusd::usd::Prim prim = stage->inner->GetPrimAtPath(p);
    if (!prim.IsValid()) {
      set_error("prim not found");
      return FREEUSD_ERR_NOT_FOUND;
    }
    const freeusd::gf::BBox3d bounds = freeusd::usdGeom::Boundable(prim).ComputeWorldBound(time);
    if (bounds.IsEmpty()) {
      set_error("boundable has no world bounds");
      return FREEUSD_ERR_NOT_FOUND;
    }
    *out_min_x = bounds.min.x();
    *out_min_y = bounds.min.y();
    *out_min_z = bounds.min.z();
    *out_max_x = bounds.max.x();
    *out_max_y = bounds.max.y();
    *out_max_z = bounds.max.z();
    clear_error();
    return FREEUSD_OK;
  } catch (const std::exception& e) {
    set_error(e.what());
    return FREEUSD_ERR_INTERNAL;
  } catch (...) {
    set_error("unknown exception");
    return FREEUSD_ERR_INTERNAL;
  }
}

int freeusd_stage_read_field_double(const FreeusdStage* stage, const char* prim_path_utf8,
                                     const char* attr_name_utf8, double time, double* out_value) {
  if (!stage || !stage->inner || !prim_path_utf8 || !attr_name_utf8 || !out_value) {
    set_error("freeusd_stage_read_field_double: null argument");
    return FREEUSD_ERR_INVALID_ARGUMENT;
  }
  try {
    const freeusd::sdf::Path p = freeusd::sdf::Path::FromString(prim_path_utf8);
    if (p.IsEmpty()) {
      set_error("invalid prim path");
      return FREEUSD_ERR_INVALID_ARGUMENT;
    }
    freeusd::vt::Value v;
    if (!stage->inner->ReadFieldAtEvaluatedTime(p, freeusd::tf::Token{attr_name_utf8}, time, &v)) {
      set_error("no value or could not evaluate attribute");
      return FREEUSD_ERR_NOT_FOUND;
    }
    double d = 0;
    if (!v.GetDouble(&d)) {
      set_error("attribute is not numeric as double");
      return FREEUSD_ERR_NOT_FOUND;
    }
    *out_value = d;
    clear_error();
    return FREEUSD_OK;
  } catch (const std::exception& e) {
    set_error(e.what());
    return FREEUSD_ERR_INTERNAL;
  } catch (...) {
    set_error("unknown exception");
    return FREEUSD_ERR_INTERNAL;
  }
}

int freeusd_stage_read_field_float(const FreeusdStage* stage, const char* prim_path_utf8,
                                   const char* attr_name_utf8, double time, float* out_value) {
  if (!stage || !stage->inner || !prim_path_utf8 || !attr_name_utf8 || !out_value) {
    set_error("freeusd_stage_read_field_float: null argument");
    return FREEUSD_ERR_INVALID_ARGUMENT;
  }
  try {
    const freeusd::sdf::Path p = freeusd::sdf::Path::FromString(prim_path_utf8);
    if (p.IsEmpty()) {
      set_error("invalid prim path");
      return FREEUSD_ERR_INVALID_ARGUMENT;
    }
    freeusd::vt::Value v;
    if (!stage->inner->ReadFieldAtEvaluatedTime(p, freeusd::tf::Token{attr_name_utf8}, time, &v)) {
      set_error("no value or could not evaluate attribute");
      return FREEUSD_ERR_NOT_FOUND;
    }
    float f = 0.0F;
    if (v.GetFloat(&f)) {
      *out_value = f;
      clear_error();
      return FREEUSD_OK;
    }
    double d = 0.0;
    if (v.GetDouble(&d)) {
      *out_value = static_cast<float>(d);
      clear_error();
      return FREEUSD_OK;
    }
    std::int32_t i32 = 0;
    if (v.GetInt32(&i32)) {
      *out_value = static_cast<float>(i32);
      clear_error();
      return FREEUSD_OK;
    }
    std::int64_t i64 = 0;
    if (v.GetInt64(&i64)) {
      *out_value = static_cast<float>(i64);
      clear_error();
      return FREEUSD_OK;
    }
    bool b = false;
    if (v.GetBool(&b)) {
      *out_value = b ? 1.0F : 0.0F;
      clear_error();
      return FREEUSD_OK;
    }
    set_error("attribute is not coercible to float");
    return FREEUSD_ERR_NOT_FOUND;
  } catch (const std::exception& e) {
    set_error(e.what());
    return FREEUSD_ERR_INTERNAL;
  } catch (...) {
    set_error("unknown exception");
    return FREEUSD_ERR_INTERNAL;
  }
}

int freeusd_stage_list_field_sample_times(const FreeusdStage* stage, const char* prim_path_utf8,
                                          const char* attr_name_utf8, double** out_times, size_t* out_count) {
  if (!stage || !stage->inner || !prim_path_utf8 || !attr_name_utf8 || !out_times || !out_count) {
    set_error("freeusd_stage_list_field_sample_times: null argument");
    return FREEUSD_ERR_INVALID_ARGUMENT;
  }
  *out_times = nullptr;
  *out_count = 0;
  try {
    const freeusd::sdf::Path p = freeusd::sdf::Path::FromString(prim_path_utf8);
    if (p.IsEmpty()) {
      set_error("invalid prim path");
      return FREEUSD_ERR_INVALID_ARGUMENT;
    }
    const std::vector<double> times =
        stage->inner->ListComposedFieldSampleTimes(p, freeusd::tf::Token{attr_name_utf8});
    if (times.empty()) {
      clear_error();
      return FREEUSD_OK;
    }
    double* arr = static_cast<double*>(std::malloc(times.size() * sizeof(double)));
    if (!arr) {
      set_error("out of memory");
      return FREEUSD_ERR_INTERNAL;
    }
    for (std::size_t i = 0; i < times.size(); ++i) {
      arr[i] = times[i];
    }
    *out_times = arr;
    *out_count = times.size();
    clear_error();
    return FREEUSD_OK;
  } catch (const std::exception& e) {
    set_error(e.what());
    return FREEUSD_ERR_INTERNAL;
  } catch (...) {
    set_error("unknown exception");
    return FREEUSD_ERR_INTERNAL;
  }
}

int freeusd_stage_has_field_opinion(const FreeusdStage* stage, const char* prim_path_utf8,
                                    const char* attr_name_utf8) {
  if (!stage || !stage->inner || !prim_path_utf8 || !attr_name_utf8) {
    set_error("freeusd_stage_has_field_opinion: null argument");
    return FREEUSD_ERR_INVALID_ARGUMENT;
  }
  try {
    const freeusd::sdf::Path p = freeusd::sdf::Path::FromString(prim_path_utf8);
    if (p.IsEmpty()) {
      set_error("invalid prim path");
      return FREEUSD_ERR_INVALID_ARGUMENT;
    }
    const int out = stage->inner->HasFieldOpinion(p, freeusd::tf::Token{attr_name_utf8}) ? 1 : 0;
    clear_error();
    return out;
  } catch (const std::exception& e) {
    set_error(e.what());
    return FREEUSD_ERR_INTERNAL;
  } catch (...) {
    set_error("unknown exception");
    return FREEUSD_ERR_INTERNAL;
  }
}

int freeusd_stage_has_attribute_connection(const FreeusdStage* stage, const char* prim_path_utf8,
                                           const char* attr_name_utf8) {
  if (!stage || !stage->inner || !prim_path_utf8 || !attr_name_utf8) {
    set_error("freeusd_stage_has_attribute_connection: null argument");
    return FREEUSD_ERR_INVALID_ARGUMENT;
  }
  try {
    const freeusd::sdf::Path p = freeusd::sdf::Path::FromString(prim_path_utf8);
    if (p.IsEmpty()) {
      set_error("invalid prim path");
      return FREEUSD_ERR_INVALID_ARGUMENT;
    }
    const int out = stage->inner->HasAttributeConnection(p, freeusd::tf::Token{attr_name_utf8}) ? 1 : 0;
    clear_error();
    return out;
  } catch (const std::exception& e) {
    set_error(e.what());
    return FREEUSD_ERR_INTERNAL;
  } catch (...) {
    set_error("unknown exception");
    return FREEUSD_ERR_INTERNAL;
  }
}

int freeusd_stage_get_attribute_connection_target(const FreeusdStage* stage, const char* prim_path_utf8,
                                                  const char* attr_name_utf8, char** out_target_utf8) {
  if (!stage || !stage->inner || !prim_path_utf8 || !attr_name_utf8 || !out_target_utf8) {
    set_error("freeusd_stage_get_attribute_connection_target: null argument");
    return FREEUSD_ERR_INVALID_ARGUMENT;
  }
  *out_target_utf8 = nullptr;
  try {
    const freeusd::sdf::Path p = freeusd::sdf::Path::FromString(prim_path_utf8);
    if (p.IsEmpty()) {
      set_error("invalid prim path");
      return FREEUSD_ERR_INVALID_ARGUMENT;
    }
    freeusd::sdf::Path tgt;
    if (!stage->inner->GetComposedAttributeConnectionTarget(p, freeusd::tf::Token{attr_name_utf8}, &tgt)) {
      set_error("no attribute connection on composed stage");
      return FREEUSD_ERR_NOT_FOUND;
    }
    const std::string s = tgt.GetString();
    *out_target_utf8 = dup_cstr(s);
    if (!*out_target_utf8) {
      set_error("out of memory");
      return FREEUSD_ERR_INTERNAL;
    }
    clear_error();
    return FREEUSD_OK;
  } catch (const std::exception& e) {
    set_error(e.what());
    return FREEUSD_ERR_INTERNAL;
  } catch (...) {
    set_error("unknown exception");
    return FREEUSD_ERR_INTERNAL;
  }
}

int freeusd_stage_read_field_bool(const FreeusdStage* stage, const char* prim_path_utf8,
                                  const char* attr_name_utf8, double time, int* out_value) {
  if (!stage || !stage->inner || !prim_path_utf8 || !attr_name_utf8 || !out_value) {
    set_error("freeusd_stage_read_field_bool: null argument");
    return FREEUSD_ERR_INVALID_ARGUMENT;
  }
  try {
    const freeusd::sdf::Path p = freeusd::sdf::Path::FromString(prim_path_utf8);
    if (p.IsEmpty()) {
      set_error("invalid prim path");
      return FREEUSD_ERR_INVALID_ARGUMENT;
    }
    freeusd::vt::Value v;
    if (!stage->inner->ReadFieldAtEvaluatedTime(p, freeusd::tf::Token{attr_name_utf8}, time, &v)) {
      set_error("no value or could not evaluate attribute");
      return FREEUSD_ERR_NOT_FOUND;
    }
    bool b = false;
    if (!v.GetBool(&b)) {
      set_error("attribute is not bool");
      return FREEUSD_ERR_NOT_FOUND;
    }
    *out_value = b ? 1 : 0;
    clear_error();
    return FREEUSD_OK;
  } catch (const std::exception& e) {
    set_error(e.what());
    return FREEUSD_ERR_INTERNAL;
  } catch (...) {
    set_error("unknown exception");
    return FREEUSD_ERR_INTERNAL;
  }
}

int freeusd_stage_read_field_int64(const FreeusdStage* stage, const char* prim_path_utf8,
                                   const char* attr_name_utf8, double time, int64_t* out_value) {
  if (!stage || !stage->inner || !prim_path_utf8 || !attr_name_utf8 || !out_value) {
    set_error("freeusd_stage_read_field_int64: null argument");
    return FREEUSD_ERR_INVALID_ARGUMENT;
  }
  try {
    const freeusd::sdf::Path p = freeusd::sdf::Path::FromString(prim_path_utf8);
    if (p.IsEmpty()) {
      set_error("invalid prim path");
      return FREEUSD_ERR_INVALID_ARGUMENT;
    }
    freeusd::vt::Value v;
    if (!stage->inner->ReadFieldAtEvaluatedTime(p, freeusd::tf::Token{attr_name_utf8}, time, &v)) {
      set_error("no value or could not evaluate attribute");
      return FREEUSD_ERR_NOT_FOUND;
    }
    std::int64_t n = 0;
    if (!value_to_int64(v, &n)) {
      set_error("attribute is not coercible to int64");
      return FREEUSD_ERR_NOT_FOUND;
    }
    *out_value = n;
    clear_error();
    return FREEUSD_OK;
  } catch (const std::exception& e) {
    set_error(e.what());
    return FREEUSD_ERR_INTERNAL;
  } catch (...) {
    set_error("unknown exception");
    return FREEUSD_ERR_INTERNAL;
  }
}

int freeusd_stage_read_field_string(const FreeusdStage* stage, const char* prim_path_utf8,
                                    const char* attr_name_utf8, double time, char** out_string) {
  if (!stage || !stage->inner || !prim_path_utf8 || !attr_name_utf8 || !out_string) {
    set_error("freeusd_stage_read_field_string: null argument");
    return FREEUSD_ERR_INVALID_ARGUMENT;
  }
  *out_string = nullptr;
  try {
    const freeusd::sdf::Path p = freeusd::sdf::Path::FromString(prim_path_utf8);
    if (p.IsEmpty()) {
      set_error("invalid prim path");
      return FREEUSD_ERR_INVALID_ARGUMENT;
    }
    freeusd::vt::Value v;
    if (!stage->inner->ReadFieldAtEvaluatedTime(p, freeusd::tf::Token{attr_name_utf8}, time, &v)) {
      set_error("no value or could not evaluate attribute");
      return FREEUSD_ERR_NOT_FOUND;
    }
    std::string s;
    if (v.GetString(&s)) {
      *out_string = dup_cstr(s);
      if (!*out_string) {
        set_error("out of memory");
        return FREEUSD_ERR_INTERNAL;
      }
      clear_error();
      return FREEUSD_OK;
    }
    freeusd::tf::Token tok;
    if (v.GetToken(&tok)) {
      s = tok.GetText();
      *out_string = dup_cstr(s);
      if (!*out_string) {
        set_error("out of memory");
        return FREEUSD_ERR_INTERNAL;
      }
      clear_error();
      return FREEUSD_OK;
    }
    set_error("attribute is not a string or token");
    return FREEUSD_ERR_NOT_FOUND;
  } catch (const std::exception& e) {
    set_error(e.what());
    return FREEUSD_ERR_INTERNAL;
  } catch (...) {
    set_error("unknown exception");
    return FREEUSD_ERR_INTERNAL;
  }
}

int freeusd_stage_read_field_vec3d(const FreeusdStage* stage, const char* prim_path_utf8,
                                   const char* attr_name_utf8, double time, double* out_x, double* out_y,
                                   double* out_z) {
  if (!stage || !stage->inner || !prim_path_utf8 || !attr_name_utf8 || !out_x || !out_y || !out_z) {
    set_error("freeusd_stage_read_field_vec3d: null argument");
    return FREEUSD_ERR_INVALID_ARGUMENT;
  }
  try {
    const freeusd::sdf::Path p = freeusd::sdf::Path::FromString(prim_path_utf8);
    if (p.IsEmpty()) {
      set_error("invalid prim path");
      return FREEUSD_ERR_INVALID_ARGUMENT;
    }
    freeusd::vt::Value v;
    if (!stage->inner->ReadFieldAtEvaluatedTime(p, freeusd::tf::Token{attr_name_utf8}, time, &v)) {
      set_error("no value or could not evaluate attribute");
      return FREEUSD_ERR_NOT_FOUND;
    }
    freeusd::gf::Vec3d vec{};
    if (!v.GetVec3d(&vec)) {
      set_error("attribute is not double3 / Vec3d");
      return FREEUSD_ERR_NOT_FOUND;
    }
    *out_x = vec.x();
    *out_y = vec.y();
    *out_z = vec.z();
    clear_error();
    return FREEUSD_OK;
  } catch (const std::exception& e) {
    set_error(e.what());
    return FREEUSD_ERR_INTERNAL;
  } catch (...) {
    set_error("unknown exception");
    return FREEUSD_ERR_INTERNAL;
  }
}

int freeusd_stage_read_field_vec3f(const FreeusdStage* stage, const char* prim_path_utf8,
                                   const char* attr_name_utf8, double time, float* out_x, float* out_y,
                                   float* out_z) {
  if (!stage || !stage->inner || !prim_path_utf8 || !attr_name_utf8 || !out_x || !out_y || !out_z) {
    set_error("freeusd_stage_read_field_vec3f: null argument");
    return FREEUSD_ERR_INVALID_ARGUMENT;
  }
  try {
    const freeusd::sdf::Path p = freeusd::sdf::Path::FromString(prim_path_utf8);
    if (p.IsEmpty()) {
      set_error("invalid prim path");
      return FREEUSD_ERR_INVALID_ARGUMENT;
    }
    freeusd::vt::Value v;
    if (!stage->inner->ReadFieldAtEvaluatedTime(p, freeusd::tf::Token{attr_name_utf8}, time, &v)) {
      set_error("no value or could not evaluate attribute");
      return FREEUSD_ERR_NOT_FOUND;
    }
    freeusd::gf::Vec3f vf{};
    if (v.GetVec3f(&vf)) {
      *out_x = vf.x();
      *out_y = vf.y();
      *out_z = vf.z();
      clear_error();
      return FREEUSD_OK;
    }
    freeusd::gf::Vec3d vd{};
    if (v.GetVec3d(&vd)) {
      *out_x = static_cast<float>(vd.x());
      *out_y = static_cast<float>(vd.y());
      *out_z = static_cast<float>(vd.z());
      clear_error();
      return FREEUSD_OK;
    }
    set_error("attribute is not float3 / vec3f or double3 / Vec3d");
    return FREEUSD_ERR_NOT_FOUND;
  } catch (const std::exception& e) {
    set_error(e.what());
    return FREEUSD_ERR_INTERNAL;
  } catch (...) {
    set_error("unknown exception");
    return FREEUSD_ERR_INTERNAL;
  }
}

int freeusd_stage_read_field_matrix4d(const FreeusdStage* stage, const char* prim_path_utf8,
                                      const char* attr_name_utf8, double time, double* out_row_major) {
  if (!stage || !stage->inner || !prim_path_utf8 || !attr_name_utf8 || !out_row_major) {
    set_error("freeusd_stage_read_field_matrix4d: null argument");
    return FREEUSD_ERR_INVALID_ARGUMENT;
  }
  try {
    const freeusd::sdf::Path p = freeusd::sdf::Path::FromString(prim_path_utf8);
    if (p.IsEmpty()) {
      set_error("invalid prim path");
      return FREEUSD_ERR_INVALID_ARGUMENT;
    }
    freeusd::vt::Value v;
    if (!stage->inner->ReadFieldAtEvaluatedTime(p, freeusd::tf::Token{attr_name_utf8}, time, &v)) {
      set_error("no value or could not evaluate attribute");
      return FREEUSD_ERR_NOT_FOUND;
    }
    freeusd::gf::Matrix4d m{};
    if (!v.GetMatrix4d(&m)) {
      set_error("attribute is not matrix4d");
      return FREEUSD_ERR_NOT_FOUND;
    }
    for (int i = 0; i < 16; ++i) {
      out_row_major[i] = m.m[static_cast<std::size_t>(i)];
    }
    clear_error();
    return FREEUSD_OK;
  } catch (const std::exception& e) {
    set_error(e.what());
    return FREEUSD_ERR_INTERNAL;
  } catch (...) {
    set_error("unknown exception");
    return FREEUSD_ERR_INTERNAL;
  }
}

int freeusd_stage_read_field_quatd(const FreeusdStage* stage, const char* prim_path_utf8,
                                   const char* attr_name_utf8, double time, double* out_real, double* out_i,
                                   double* out_j, double* out_k) {
  if (!stage || !stage->inner || !prim_path_utf8 || !attr_name_utf8 || !out_real || !out_i || !out_j || !out_k) {
    set_error("freeusd_stage_read_field_quatd: null argument");
    return FREEUSD_ERR_INVALID_ARGUMENT;
  }
  try {
    const freeusd::sdf::Path p = freeusd::sdf::Path::FromString(prim_path_utf8);
    if (p.IsEmpty()) {
      set_error("invalid prim path");
      return FREEUSD_ERR_INVALID_ARGUMENT;
    }
    freeusd::vt::Value v;
    if (!stage->inner->ReadFieldAtEvaluatedTime(p, freeusd::tf::Token{attr_name_utf8}, time, &v)) {
      set_error("no value or could not evaluate attribute");
      return FREEUSD_ERR_NOT_FOUND;
    }
    freeusd::gf::Quatd q{};
    if (!v.GetQuatd(&q)) {
      set_error("attribute is not quatd");
      return FREEUSD_ERR_NOT_FOUND;
    }
    *out_real = q.real;
    *out_i = q.i;
    *out_j = q.j;
    *out_k = q.k;
    clear_error();
    return FREEUSD_OK;
  } catch (const std::exception& e) {
    set_error(e.what());
    return FREEUSD_ERR_INTERNAL;
  } catch (...) {
    set_error("unknown exception");
    return FREEUSD_ERR_INTERNAL;
  }
}

int freeusd_stage_read_field_quatf(const FreeusdStage* stage, const char* prim_path_utf8,
                                   const char* attr_name_utf8, double time, float* out_real, float* out_i,
                                   float* out_j, float* out_k) {
  if (!stage || !stage->inner || !prim_path_utf8 || !attr_name_utf8 || !out_real || !out_i || !out_j || !out_k) {
    set_error("freeusd_stage_read_field_quatf: null argument");
    return FREEUSD_ERR_INVALID_ARGUMENT;
  }
  try {
    const freeusd::sdf::Path p = freeusd::sdf::Path::FromString(prim_path_utf8);
    if (p.IsEmpty()) {
      set_error("invalid prim path");
      return FREEUSD_ERR_INVALID_ARGUMENT;
    }
    freeusd::vt::Value v;
    if (!stage->inner->ReadFieldAtEvaluatedTime(p, freeusd::tf::Token{attr_name_utf8}, time, &v)) {
      set_error("no value or could not evaluate attribute");
      return FREEUSD_ERR_NOT_FOUND;
    }
    freeusd::gf::Quatf qf{};
    if (v.GetQuatf(&qf)) {
      *out_real = qf.real;
      *out_i = qf.i;
      *out_j = qf.j;
      *out_k = qf.k;
      clear_error();
      return FREEUSD_OK;
    }
    freeusd::gf::Quatd qd{};
    if (v.GetQuatd(&qd)) {
      *out_real = static_cast<float>(qd.real);
      *out_i = static_cast<float>(qd.i);
      *out_j = static_cast<float>(qd.j);
      *out_k = static_cast<float>(qd.k);
      clear_error();
      return FREEUSD_OK;
    }
    set_error("attribute is not quatf or quatd");
    return FREEUSD_ERR_NOT_FOUND;
  } catch (const std::exception& e) {
    set_error(e.what());
    return FREEUSD_ERR_INTERNAL;
  } catch (...) {
    set_error("unknown exception");
    return FREEUSD_ERR_INTERNAL;
  }
}

int freeusd_stage_read_field_token(const FreeusdStage* stage, const char* prim_path_utf8,
                                   const char* attr_name_utf8, double time, char** out_token_utf8) {
  if (!stage || !stage->inner || !prim_path_utf8 || !attr_name_utf8 || !out_token_utf8) {
    set_error("freeusd_stage_read_field_token: null argument");
    return FREEUSD_ERR_INVALID_ARGUMENT;
  }
  *out_token_utf8 = nullptr;
  try {
    const freeusd::sdf::Path p = freeusd::sdf::Path::FromString(prim_path_utf8);
    if (p.IsEmpty()) {
      set_error("invalid prim path");
      return FREEUSD_ERR_INVALID_ARGUMENT;
    }
    freeusd::vt::Value v;
    if (!stage->inner->ReadFieldAtEvaluatedTime(p, freeusd::tf::Token{attr_name_utf8}, time, &v)) {
      set_error("no value or could not evaluate attribute");
      return FREEUSD_ERR_NOT_FOUND;
    }
    freeusd::tf::Token tok;
    if (!v.GetToken(&tok)) {
      set_error("attribute is not a token");
      return FREEUSD_ERR_NOT_FOUND;
    }
    *out_token_utf8 = dup_cstr(std::string{tok.GetText()});
    if (!*out_token_utf8) {
      set_error("out of memory");
      return FREEUSD_ERR_INTERNAL;
    }
    clear_error();
    return FREEUSD_OK;
  } catch (const std::exception& e) {
    set_error(e.what());
    return FREEUSD_ERR_INTERNAL;
  } catch (...) {
    set_error("unknown exception");
    return FREEUSD_ERR_INTERNAL;
  }
}

int freeusd_stage_read_field_token_array(const FreeusdStage* stage, const char* prim_path_utf8,
                                         const char* attr_name_utf8, double time, char*** out_strings,
                                         size_t* out_count) {
  if (!stage || !stage->inner || !prim_path_utf8 || !attr_name_utf8 || !out_strings || !out_count) {
    set_error("freeusd_stage_read_field_token_array: null argument");
    return FREEUSD_ERR_INVALID_ARGUMENT;
  }
  *out_strings = nullptr;
  *out_count = 0;
  try {
    const freeusd::sdf::Path p = freeusd::sdf::Path::FromString(prim_path_utf8);
    if (p.IsEmpty()) {
      set_error("invalid prim path");
      return FREEUSD_ERR_INVALID_ARGUMENT;
    }
    freeusd::vt::Value v;
    if (!stage->inner->ReadFieldAtEvaluatedTime(p, freeusd::tf::Token{attr_name_utf8}, time, &v)) {
      set_error("no value or could not evaluate attribute");
      return FREEUSD_ERR_NOT_FOUND;
    }
    std::vector<freeusd::tf::Token> elems;
    if (!v.GetTokenArray(&elems)) {
      set_error("attribute is not token[]");
      return FREEUSD_ERR_NOT_FOUND;
    }
    std::vector<std::string> strs;
    strs.reserve(elems.size());
    for (const auto& t : elems) {
      strs.emplace_back(t.GetText());
    }
    const int ml = malloc_string_list(strs, out_strings, out_count);
    if (ml != FREEUSD_OK) {
      if (ml == FREEUSD_ERR_INTERNAL) {
        set_error("out of memory");
      }
      return ml;
    }
    clear_error();
    return FREEUSD_OK;
  } catch (const std::exception& e) {
    set_error(e.what());
    return FREEUSD_ERR_INTERNAL;
  } catch (...) {
    set_error("unknown exception");
    return FREEUSD_ERR_INTERNAL;
  }
}

int freeusd_stage_read_geom_blend_shape_target_count(const FreeusdStage* stage, const char* geom_path_utf8,
                                                     size_t* out_count) {
  if (!stage || !stage->inner || !geom_path_utf8 || !out_count) {
    set_error("freeusd_stage_read_geom_blend_shape_target_count: null argument");
    return FREEUSD_ERR_INVALID_ARGUMENT;
  }
  *out_count = 0;
  try {
    const freeusd::sdf::Path p = freeusd::sdf::Path::FromString(geom_path_utf8);
    if (p.IsEmpty()) {
      set_error("invalid geom path");
      return FREEUSD_ERR_INVALID_ARGUMENT;
    }
    const freeusd::usdSkel::SkelBlendShapes binding =
        freeusd::usdSkel::SkelBlendShapes::ReadFromGeomPrim(stage->inner->GetPrimAtPath(p));
    if (!binding) {
      set_error("geom prim has no blend shape binding");
      return FREEUSD_ERR_NOT_FOUND;
    }
    *out_count = binding.blend_shape_tokens.size();
    clear_error();
    return FREEUSD_OK;
  } catch (const std::exception& e) {
    set_error(e.what());
    return FREEUSD_ERR_INTERNAL;
  } catch (...) {
    set_error("unknown exception");
    return FREEUSD_ERR_INTERNAL;
  }
}

int freeusd_stage_read_geom_blend_shape_weight(const FreeusdStage* stage, const char* geom_path_utf8,
                                               size_t target_index, double time, float* out_weight) {
  if (!stage || !stage->inner || !geom_path_utf8 || !out_weight) {
    set_error("freeusd_stage_read_geom_blend_shape_weight: null argument");
    return FREEUSD_ERR_INVALID_ARGUMENT;
  }
  *out_weight = 0.0f;
  try {
    const freeusd::sdf::Path p = freeusd::sdf::Path::FromString(geom_path_utf8);
    if (p.IsEmpty()) {
      set_error("invalid geom path");
      return FREEUSD_ERR_INVALID_ARGUMENT;
    }
    const freeusd::usdSkel::SkelBlendShapes binding =
        freeusd::usdSkel::SkelBlendShapes::ReadFromGeomPrim(stage->inner->GetPrimAtPath(p));
    if (!binding) {
      set_error("geom prim has no blend shape binding");
      return FREEUSD_ERR_NOT_FOUND;
    }
    std::vector<float> weights;
    if (!binding.GetWeights(&weights, time)) {
      set_error("blend shape weights not available");
      return FREEUSD_ERR_NOT_FOUND;
    }
    if (target_index >= weights.size()) {
      set_error("blend shape target index out of range");
      return FREEUSD_ERR_NOT_FOUND;
    }
    *out_weight = weights[target_index];
    clear_error();
    return FREEUSD_OK;
  } catch (const std::exception& e) {
    set_error(e.what());
    return FREEUSD_ERR_INTERNAL;
  } catch (...) {
    set_error("unknown exception");
    return FREEUSD_ERR_INTERNAL;
  }
}

int freeusd_usdskel_compute_skinning_matrices(size_t joint_count, const double* joint_world_row_major,
                                              const double* inverse_bind_row_major, double* out_palette_row_major) {
  if (joint_count == 0 || !joint_world_row_major || !inverse_bind_row_major || !out_palette_row_major) {
    set_error("freeusd_usdskel_compute_skinning_matrices: null argument or zero joint_count");
    return FREEUSD_ERR_INVALID_ARGUMENT;
  }
  try {
    std::vector<freeusd::gf::Matrix4d> world;
    std::vector<freeusd::gf::Matrix4d> bind;
    world.reserve(joint_count);
    bind.reserve(joint_count);
    for (std::size_t i = 0; i < joint_count; ++i) {
      freeusd::gf::Matrix4d w{};
      freeusd::gf::Matrix4d b{};
      const double* wsrc = joint_world_row_major + i * 16;
      const double* bsrc = inverse_bind_row_major + i * 16;
      for (std::size_t k = 0; k < 16; ++k) {
        w.m[k] = wsrc[k];
        b.m[k] = bsrc[k];
      }
      world.push_back(w);
      bind.push_back(b);
    }
    std::vector<freeusd::gf::Matrix4d> palette;
    if (!freeusd::usdSkel::ComputeSkinningMatrices(world, bind, &palette)) {
      set_error("compute skinning matrices failed");
      return FREEUSD_ERR_INTERNAL;
    }
    for (std::size_t i = 0; i < palette.size(); ++i) {
      double* dst = out_palette_row_major + i * 16;
      for (std::size_t k = 0; k < 16; ++k) {
        dst[k] = palette[i].m[k];
      }
    }
    clear_error();
    return FREEUSD_OK;
  } catch (const std::exception& e) {
    set_error(e.what());
    return FREEUSD_ERR_INTERNAL;
  } catch (...) {
    set_error("unknown exception");
    return FREEUSD_ERR_INTERNAL;
  }
}

int freeusd_stage_deform_points_with_skeleton(const FreeusdStage* stage, const char* skeleton_path_utf8,
                                              const char* animation_path_utf8, double time, size_t point_count,
                                              const float* in_points_xyz, const int* joint_indices,
                                              const float* joint_weights, size_t influences_per_point,
                                              float* out_points_xyz) {
  if (!stage || !stage->inner || !skeleton_path_utf8 || !animation_path_utf8 || point_count == 0 ||
      !in_points_xyz || !joint_indices || !joint_weights || !out_points_xyz) {
    set_error("freeusd_stage_deform_points_with_skeleton: null argument or zero point_count");
    return FREEUSD_ERR_INVALID_ARGUMENT;
  }
  if (point_count > 1'000'000) {
    set_error("point_count exceeds limit");
    return FREEUSD_ERR_INVALID_ARGUMENT;
  }
  try {
    const freeusd::sdf::Path skel_path = freeusd::sdf::Path::FromString(skeleton_path_utf8);
    const freeusd::sdf::Path anim_path = freeusd::sdf::Path::FromString(animation_path_utf8);
    if (skel_path.IsEmpty() || anim_path.IsEmpty()) {
      set_error("invalid skeleton or animation path");
      return FREEUSD_ERR_INVALID_ARGUMENT;
    }
    const freeusd::usdSkel::Skeleton skel = freeusd::usdSkel::Skeleton::ReadFromPrim(stage->inner, skel_path);
    const freeusd::usdSkel::SkelAnimation anim(stage->inner->GetPrimAtPath(anim_path));
    if (!skel || !anim) {
      set_error("skeleton or animation prim not found");
      return FREEUSD_ERR_NOT_FOUND;
    }
    std::vector<freeusd::gf::Matrix4d> bind{};
    if (!skel.GetBindTransforms(&bind, time)) {
      set_error("skeleton bind transforms unavailable");
      return FREEUSD_ERR_NOT_FOUND;
    }
    std::vector<freeusd::gf::Matrix4d> joint_world{};
    if (!freeusd::usdSkel::BuildJointWorldMatricesFromAnimation(skel, anim, time, &joint_world)) {
      set_error("joint world matrices unavailable");
      return FREEUSD_ERR_NOT_FOUND;
    }
    std::vector<freeusd::gf::Vec3f> points;
    points.reserve(point_count);
    for (std::size_t i = 0; i < point_count; ++i) {
      freeusd::gf::Vec3f v{};
      v.set(in_points_xyz[i * 3 + 0], in_points_xyz[i * 3 + 1], in_points_xyz[i * 3 + 2]);
      points.push_back(v);
    }
    const std::vector<int> indices(joint_indices, joint_indices + point_count * influences_per_point);
    const std::vector<float> weights(joint_weights, joint_weights + point_count * influences_per_point);
    std::vector<freeusd::gf::Vec3f> deformed;
    if (!freeusd::usdSkel::DeformPointsWithSkeleton(points, indices, weights, influences_per_point, joint_world, bind,
                                                    nullptr, &deformed)) {
      set_error("deform points with skeleton failed");
      return FREEUSD_ERR_INTERNAL;
    }
    for (std::size_t i = 0; i < deformed.size(); ++i) {
      out_points_xyz[i * 3 + 0] = deformed[i].x();
      out_points_xyz[i * 3 + 1] = deformed[i].y();
      out_points_xyz[i * 3 + 2] = deformed[i].z();
    }
    clear_error();
    return FREEUSD_OK;
  } catch (const std::exception& e) {
    set_error(e.what());
    return FREEUSD_ERR_INTERNAL;
  } catch (...) {
    set_error("unknown exception");
    return FREEUSD_ERR_INTERNAL;
  }
}

int freeusd_usdutils_assess_engine_runtime_support(const FreeusdStage* stage, FreeusdEngineRuntimeSupport* out) {
  if (!stage || !stage->inner || !out) {
    set_error("freeusd_usdutils_assess_engine_runtime_support: null argument");
    return FREEUSD_ERR_INVALID_ARGUMENT;
  }
  try {
    const freeusd::usdUtils::EngineRuntimeSupportReport report =
        freeusd::usdUtils::AssessEngineRuntimeSupport(*stage->inner);
    out->recommended_mode = static_cast<int>(report.recommended_mode);
    out->uses_composed_layer_stack = report.uses_composed_layer_stack ? 1 : 0;
    out->uses_references = report.uses_references ? 1 : 0;
    out->uses_payloads = report.uses_payloads ? 1 : 0;
    out->uses_inherits = report.uses_inherits ? 1 : 0;
    out->uses_specializes = report.uses_specializes ? 1 : 0;
    out->uses_variant_selection = report.uses_variant_selection ? 1 : 0;
    out->uses_variant_sets = report.uses_variant_sets ? 1 : 0;
    out->uses_relocates = report.uses_relocates ? 1 : 0;
    out->uses_prefix_substitutions = report.uses_prefix_substitutions ? 1 : 0;
    out->uses_time_samples = report.uses_time_samples ? 1 : 0;
    out->uses_relationships = report.uses_relationships ? 1 : 0;
    out->uses_custom_data = report.uses_custom_data ? 1 : 0;
    out->uses_attribute_connections = report.uses_attribute_connections ? 1 : 0;
    out->uses_skel_bound_meshes = report.uses_skel_bound_meshes ? 1 : 0;
    out->uses_blend_shapes = report.uses_blend_shapes ? 1 : 0;
    out->uses_skel_animation = report.uses_skel_animation ? 1 : 0;
    out->uses_material_bindings = report.uses_material_bindings ? 1 : 0;
    out->uses_preview_surface = report.uses_preview_surface ? 1 : 0;
    clear_error();
    return FREEUSD_OK;
  } catch (const std::exception& e) {
    set_error(e.what());
    return FREEUSD_ERR_INTERNAL;
  } catch (...) {
    set_error("unknown exception");
    return FREEUSD_ERR_INTERNAL;
  }
}

int freeusd_stage_read_material_surface_shader_path(const FreeusdStage* stage, const char* material_path_utf8,
                                                    char** out_shader_path_utf8) {
  if (!stage || !stage->inner || !material_path_utf8 || !out_shader_path_utf8) {
    set_error("freeusd_stage_read_material_surface_shader_path: null argument");
    return FREEUSD_ERR_INVALID_ARGUMENT;
  }
  *out_shader_path_utf8 = nullptr;
  try {
    const freeusd::sdf::Path p = freeusd::sdf::Path::FromString(material_path_utf8);
    if (p.IsEmpty()) {
      set_error("invalid material path");
      return FREEUSD_ERR_INVALID_ARGUMENT;
    }
    const freeusd::usdShade::Material mat = freeusd::usdShade::Material::ReadFromPrim(stage->inner, p);
    if (!mat) {
      set_error("material prim not found or invalid");
      return FREEUSD_ERR_NOT_FOUND;
    }
    const freeusd::sdf::Path shader = mat.GetSurfaceShaderPath();
    if (shader.IsEmpty()) {
      set_error("material has no outputs:surface connection");
      return FREEUSD_ERR_NOT_FOUND;
    }
    char* dup = dup_cstr(shader.GetString());
    if (!dup) {
      set_error("out of memory");
      return FREEUSD_ERR_INTERNAL;
    }
    *out_shader_path_utf8 = dup;
    clear_error();
    return FREEUSD_OK;
  } catch (const std::exception& e) {
    set_error(e.what());
    return FREEUSD_ERR_INTERNAL;
  } catch (...) {
    set_error("unknown exception");
    return FREEUSD_ERR_INTERNAL;
  }
}

int freeusd_stage_read_preview_surface_diffuse_color(const FreeusdStage* stage, const char* shader_path_utf8,
                                                     double time, float out_rgb[3]) {
  if (!stage || !stage->inner || !shader_path_utf8 || !out_rgb) {
    set_error("freeusd_stage_read_preview_surface_diffuse_color: null argument");
    return FREEUSD_ERR_INVALID_ARGUMENT;
  }
  out_rgb[0] = out_rgb[1] = out_rgb[2] = 0.0f;
  try {
    const freeusd::sdf::Path p = freeusd::sdf::Path::FromString(shader_path_utf8);
    if (p.IsEmpty()) {
      set_error("invalid shader path");
      return FREEUSD_ERR_INVALID_ARGUMENT;
    }
    const freeusd::usdShade::PreviewSurface preview =
        freeusd::usdShade::PreviewSurface::ReadFromPrim(stage->inner, p);
    if (!preview || !preview.IsPreviewSurface()) {
      set_error("shader prim is not UsdPreviewSurface");
      return FREEUSD_ERR_NOT_FOUND;
    }
    freeusd::gf::Vec3f diffuse{};
    if (!preview.GetDiffuseColor(&diffuse, time)) {
      set_error("inputs:diffuseColor not available");
      return FREEUSD_ERR_NOT_FOUND;
    }
    out_rgb[0] = diffuse.x();
    out_rgb[1] = diffuse.y();
    out_rgb[2] = diffuse.z();
    clear_error();
    return FREEUSD_OK;
  } catch (const std::exception& e) {
    set_error(e.what());
    return FREEUSD_ERR_INTERNAL;
  } catch (...) {
    set_error("unknown exception");
    return FREEUSD_ERR_INTERNAL;
  }
}

int freeusd_stage_read_skel_joint_names(const FreeusdStage* stage, const char* skeleton_path_utf8, char*** out_strings,
                                        size_t* out_count) {
  if (!stage || !stage->inner || !skeleton_path_utf8 || !out_strings || !out_count) {
    set_error("freeusd_stage_read_skel_joint_names: null argument");
    return FREEUSD_ERR_INVALID_ARGUMENT;
  }
  *out_strings = nullptr;
  *out_count = 0;
  try {
    const freeusd::sdf::Path p = freeusd::sdf::Path::FromString(skeleton_path_utf8);
    if (p.IsEmpty()) {
      set_error("invalid skeleton path");
      return FREEUSD_ERR_INVALID_ARGUMENT;
    }
    const freeusd::usdSkel::Skeleton skel =
        freeusd::usdSkel::Skeleton::ReadFromPrim(stage->inner, p);
    if (!skel) {
      set_error("skeleton prim not found or invalid");
      return FREEUSD_ERR_NOT_FOUND;
    }
    const std::vector<std::string> names = skel.GetJointNames();
    const int ml = malloc_string_list(names, out_strings, out_count);
    if (ml != FREEUSD_OK) {
      if (ml == FREEUSD_ERR_INTERNAL) {
        set_error("out of memory");
      }
      return ml;
    }
    clear_error();
    return FREEUSD_OK;
  } catch (const std::exception& e) {
    set_error(e.what());
    return FREEUSD_ERR_INTERNAL;
  } catch (...) {
    set_error("unknown exception");
    return FREEUSD_ERR_INTERNAL;
  }
}

int freeusd_stage_resolve_prim_active(const FreeusdStage* stage, const char* prim_path_utf8, int* out_active) {
  if (!stage || !stage->inner || !prim_path_utf8 || !out_active) {
    set_error("freeusd_stage_resolve_prim_active: null argument");
    return FREEUSD_ERR_INVALID_ARGUMENT;
  }
  try {
    const freeusd::sdf::Path p = freeusd::sdf::Path::FromString(prim_path_utf8);
    if (p.IsEmpty()) {
      set_error("invalid prim path");
      return FREEUSD_ERR_INVALID_ARGUMENT;
    }
    *out_active = stage->inner->ResolvePrimActive(p) ? 1 : 0;
    clear_error();
    return FREEUSD_OK;
  } catch (const std::exception& e) {
    set_error(e.what());
    return FREEUSD_ERR_INTERNAL;
  } catch (...) {
    set_error("unknown exception");
    return FREEUSD_ERR_INTERNAL;
  }
}

int freeusd_stage_resolve_has_prim_active_opinion(const FreeusdStage* stage, const char* prim_path_utf8) {
  if (!stage || !stage->inner || !prim_path_utf8) {
    set_error("freeusd_stage_resolve_has_prim_active_opinion: null argument");
    return FREEUSD_ERR_INVALID_ARGUMENT;
  }
  try {
    const freeusd::sdf::Path p = freeusd::sdf::Path::FromString(prim_path_utf8);
    if (p.IsEmpty()) {
      set_error("invalid prim path");
      return FREEUSD_ERR_INVALID_ARGUMENT;
    }
    const int out = stage->inner->ResolveHasPrimActiveOpinion(p) ? 1 : 0;
    clear_error();
    return out;
  } catch (const std::exception& e) {
    set_error(e.what());
    return FREEUSD_ERR_INTERNAL;
  } catch (...) {
    set_error("unknown exception");
    return FREEUSD_ERR_INTERNAL;
  }
}

int freeusd_stage_resolve_prim_specifier_kind(const FreeusdStage* stage, const char* prim_path_utf8) {
  if (!stage || !stage->inner || !prim_path_utf8) {
    set_error("freeusd_stage_resolve_prim_specifier_kind: null argument");
    return -FREEUSD_ERR_INVALID_ARGUMENT;
  }
  try {
    const freeusd::sdf::Path p = freeusd::sdf::Path::FromString(prim_path_utf8);
    if (p.IsEmpty() || !p.IsPrimPath()) {
      set_error("invalid prim path");
      return -FREEUSD_ERR_INVALID_ARGUMENT;
    }
    const int k = static_cast<int>(stage->inner->ResolvePrimSpecifierKind(p));
    clear_error();
    return k;
  } catch (const std::exception& e) {
    set_error(e.what());
    return -FREEUSD_ERR_INTERNAL;
  } catch (...) {
    set_error("unknown exception");
    return -FREEUSD_ERR_INTERNAL;
  }
}

char* freeusd_stage_resolve_prim_kind(const FreeusdStage* stage, const char* prim_path_utf8) {
  if (!stage || !stage->inner || !prim_path_utf8) {
    set_error("freeusd_stage_resolve_prim_kind: null argument");
    return nullptr;
  }
  try {
    const freeusd::sdf::Path p = freeusd::sdf::Path::FromString(prim_path_utf8);
    if (p.IsEmpty()) {
      set_error("invalid prim path");
      return nullptr;
    }
    if (!stage->inner->ResolveHasPrimKind(p)) {
      clear_error();
      return nullptr;
    }
    const freeusd::tf::Token t = stage->inner->ResolvePrimKind(p);
    const std::string s = t.GetText();
    if (s.empty()) {
      clear_error();
      return nullptr;
    }
    clear_error();
    return dup_cstr(s);
  } catch (const std::exception& e) {
    set_error(e.what());
    return nullptr;
  } catch (...) {
    set_error("unknown exception");
    return nullptr;
  }
}

int freeusd_stage_resolve_has_prim_kind(const FreeusdStage* stage, const char* prim_path_utf8) {
  if (!stage || !stage->inner || !prim_path_utf8) {
    set_error("freeusd_stage_resolve_has_prim_kind: null argument");
    return FREEUSD_ERR_INVALID_ARGUMENT;
  }
  try {
    const freeusd::sdf::Path p = freeusd::sdf::Path::FromString(prim_path_utf8);
    if (p.IsEmpty()) {
      set_error("invalid prim path");
      return FREEUSD_ERR_INVALID_ARGUMENT;
    }
    const int out = stage->inner->ResolveHasPrimKind(p) ? 1 : 0;
    clear_error();
    return out;
  } catch (const std::exception& e) {
    set_error(e.what());
    return FREEUSD_ERR_INTERNAL;
  } catch (...) {
    set_error("unknown exception");
    return FREEUSD_ERR_INTERNAL;
  }
}

int freeusd_stage_get_composed_prim_custom_data(const FreeusdStage* stage, const char* prim_path_utf8,
                                                const char* key_utf8, char** out_value) {
  if (!stage || !stage->inner || !prim_path_utf8 || !key_utf8 || !out_value) {
    set_error("freeusd_stage_get_composed_prim_custom_data: null argument");
    return FREEUSD_ERR_INVALID_ARGUMENT;
  }
  *out_value = nullptr;
  try {
    const freeusd::sdf::Path p = freeusd::sdf::Path::FromString(prim_path_utf8);
    if (p.IsEmpty()) {
      set_error("invalid prim path");
      return FREEUSD_ERR_INVALID_ARGUMENT;
    }
    freeusd::vt::Value v;
    if (!stage->inner->GetComposedPrimCustomData(p, std::string{key_utf8}, &v)) {
      set_error("no customData key on composed prim");
      return FREEUSD_ERR_NOT_FOUND;
    }
    std::string s;
    if (v.GetString(&s)) {
      *out_value = dup_cstr(s);
      if (!*out_value) {
        set_error("out of memory");
        return FREEUSD_ERR_INTERNAL;
      }
      clear_error();
      return FREEUSD_OK;
    }
    freeusd::tf::Token tok;
    if (v.GetToken(&tok)) {
      s = tok.GetText();
      *out_value = dup_cstr(s);
      if (!*out_value) {
        set_error("out of memory");
        return FREEUSD_ERR_INTERNAL;
      }
      clear_error();
      return FREEUSD_OK;
    }
    set_error("customData value is not string or token (use C++ API for other types)");
    return FREEUSD_ERR_NOT_FOUND;
  } catch (const std::exception& e) {
    set_error(e.what());
    return FREEUSD_ERR_INTERNAL;
  } catch (...) {
    set_error("unknown exception");
    return FREEUSD_ERR_INTERNAL;
  }
}

int freeusd_stage_get_composed_prim_custom_data_int64(const FreeusdStage* stage, const char* prim_path_utf8,
                                                      const char* key_utf8, int64_t* out_value) {
  if (!stage || !stage->inner || !prim_path_utf8 || !key_utf8 || !out_value) {
    set_error("freeusd_stage_get_composed_prim_custom_data_int64: null argument");
    return FREEUSD_ERR_INVALID_ARGUMENT;
  }
  try {
    const freeusd::sdf::Path p = freeusd::sdf::Path::FromString(prim_path_utf8);
    if (p.IsEmpty()) {
      set_error("invalid prim path");
      return FREEUSD_ERR_INVALID_ARGUMENT;
    }
    freeusd::vt::Value v;
    if (!stage->inner->GetComposedPrimCustomData(p, std::string{key_utf8}, &v)) {
      set_error("no customData key on composed prim");
      return FREEUSD_ERR_NOT_FOUND;
    }
    std::int64_t i64 = 0;
    if (v.GetInt64(&i64)) {
      *out_value = i64;
      clear_error();
      return FREEUSD_OK;
    }
    std::int32_t i32 = 0;
    if (v.GetInt32(&i32)) {
      *out_value = static_cast<int64_t>(i32);
      clear_error();
      return FREEUSD_OK;
    }
    bool b = false;
    if (v.GetBool(&b)) {
      *out_value = b ? 1 : 0;
      clear_error();
      return FREEUSD_OK;
    }
    set_error("customData value is not int or bool (use string API or C++ for other types)");
    return FREEUSD_ERR_NOT_FOUND;
  } catch (const std::exception& e) {
    set_error(e.what());
    return FREEUSD_ERR_INTERNAL;
  } catch (...) {
    set_error("unknown exception");
    return FREEUSD_ERR_INTERNAL;
  }
}

int freeusd_stage_prim_custom_data_key_in_any_layer(const FreeusdStage* stage, const char* prim_path_utf8,
                                                     const char* key_utf8) {
  if (!stage || !stage->inner || !prim_path_utf8 || !key_utf8) {
    set_error("freeusd_stage_prim_custom_data_key_in_any_layer: null argument");
    return FREEUSD_ERR_INVALID_ARGUMENT;
  }
  if (key_utf8[0] == '\0') {
    set_error("empty customData key");
    return FREEUSD_ERR_INVALID_ARGUMENT;
  }
  try {
    const freeusd::sdf::Path p = freeusd::sdf::Path::FromString(prim_path_utf8);
    if (p.IsEmpty()) {
      set_error("invalid prim path");
      return FREEUSD_ERR_INVALID_ARGUMENT;
    }
    const int out = stage->inner->PrimCustomDataKeyInAnyLayer(p, std::string{key_utf8}) ? 1 : 0;
    clear_error();
    return out;
  } catch (const std::exception& e) {
    set_error(e.what());
    return FREEUSD_ERR_INTERNAL;
  } catch (...) {
    set_error("unknown exception");
    return FREEUSD_ERR_INTERNAL;
  }
}

int freeusd_stage_list_composed_prim_custom_data_keys(const FreeusdStage* stage, const char* prim_path_utf8,
                                                      char*** out_keys, size_t* out_count) {
  if (!stage || !stage->inner || !prim_path_utf8 || !out_keys || !out_count) {
    set_error("freeusd_stage_list_composed_prim_custom_data_keys: null argument");
    return FREEUSD_ERR_INVALID_ARGUMENT;
  }
  *out_keys = nullptr;
  *out_count = 0;
  try {
    const freeusd::sdf::Path p = freeusd::sdf::Path::FromString(prim_path_utf8);
    if (p.IsEmpty()) {
      set_error("invalid prim path");
      return FREEUSD_ERR_INVALID_ARGUMENT;
    }
    const std::vector<std::string> keys = stage->inner->ListComposedPrimCustomDataKeys(p);
    if (keys.empty()) {
      clear_error();
      return FREEUSD_OK;
    }
    char** arr = static_cast<char**>(std::malloc(keys.size() * sizeof(char*)));
    if (!arr) {
      set_error("out of memory");
      return FREEUSD_ERR_INTERNAL;
    }
    for (std::size_t i = 0; i < keys.size(); ++i) {
      arr[i] = dup_cstr(keys[i]);
      if (!arr[i]) {
        for (std::size_t j = 0; j < i; ++j) {
          std::free(arr[j]);
        }
        std::free(arr);
        set_error("out of memory");
        return FREEUSD_ERR_INTERNAL;
      }
    }
    *out_keys = arr;
    *out_count = keys.size();
    clear_error();
    return FREEUSD_OK;
  } catch (const std::exception& e) {
    set_error(e.what());
    return FREEUSD_ERR_INTERNAL;
  } catch (...) {
    set_error("unknown exception");
    return FREEUSD_ERR_INTERNAL;
  }
}

int freeusd_stage_get_composed_custom_layer_data_utf8(const FreeusdStage* stage, const char* key_utf8, char** out_value) {
  if (!stage || !stage->inner || !key_utf8 || !out_value) {
    set_error("freeusd_stage_get_composed_custom_layer_data_utf8: null argument");
    return FREEUSD_ERR_INVALID_ARGUMENT;
  }
  *out_value = nullptr;
  if (key_utf8[0] == '\0') {
    set_error("empty customLayerData key");
    return FREEUSD_ERR_INVALID_ARGUMENT;
  }
  try {
    freeusd::vt::Value v;
    if (!stage->inner->GetComposedCustomLayerData(std::string{key_utf8}, &v)) {
      set_error("no customLayerData key on composed stage");
      return FREEUSD_ERR_NOT_FOUND;
    }
    std::string s;
    if (v.GetString(&s)) {
      *out_value = dup_cstr(s);
      if (!*out_value) {
        set_error("out of memory");
        return FREEUSD_ERR_INTERNAL;
      }
      clear_error();
      return FREEUSD_OK;
    }
    freeusd::tf::Token tok;
    if (v.GetToken(&tok)) {
      s = tok.GetText();
      *out_value = dup_cstr(s);
      if (!*out_value) {
        set_error("out of memory");
        return FREEUSD_ERR_INTERNAL;
      }
      clear_error();
      return FREEUSD_OK;
    }
    set_error("customLayerData value is not string or token (use C++ API for other types)");
    return FREEUSD_ERR_NOT_FOUND;
  } catch (const std::exception& e) {
    set_error(e.what());
    return FREEUSD_ERR_INTERNAL;
  } catch (...) {
    set_error("unknown exception");
    return FREEUSD_ERR_INTERNAL;
  }
}

int freeusd_stage_custom_layer_data_key_in_any_layer(const FreeusdStage* stage, const char* key_utf8) {
  if (!stage || !stage->inner || !key_utf8) {
    set_error("freeusd_stage_custom_layer_data_key_in_any_layer: null argument");
    return FREEUSD_ERR_INVALID_ARGUMENT;
  }
  try {
    const int out = stage->inner->CustomLayerDataKeyInAnyLayer(std::string{key_utf8}) ? 1 : 0;
    clear_error();
    return out;
  } catch (const std::exception& e) {
    set_error(e.what());
    return FREEUSD_ERR_INTERNAL;
  } catch (...) {
    set_error("unknown exception");
    return FREEUSD_ERR_INTERNAL;
  }
}

int freeusd_stage_list_composed_custom_layer_data_keys(const FreeusdStage* stage, char*** out_keys, size_t* out_count) {
  if (!stage || !stage->inner || !out_keys || !out_count) {
    set_error("freeusd_stage_list_composed_custom_layer_data_keys: null argument");
    return FREEUSD_ERR_INVALID_ARGUMENT;
  }
  *out_keys = nullptr;
  *out_count = 0;
  try {
    const std::vector<std::string> keys = stage->inner->ListComposedCustomLayerDataKeys();
    const int rc = malloc_string_list(keys, out_keys, out_count);
    if (rc != FREEUSD_OK) {
      set_error(rc == FREEUSD_ERR_INTERNAL ? "out of memory" : "invalid argument");
      return rc;
    }
    clear_error();
    return FREEUSD_OK;
  } catch (const std::exception& e) {
    set_error(e.what());
    return FREEUSD_ERR_INTERNAL;
  } catch (...) {
    set_error("unknown exception");
    return FREEUSD_ERR_INTERNAL;
  }
}

int freeusd_stage_get_composed_prim_variant_selection_utf8(const FreeusdStage* stage, const char* prim_path_utf8,
                                                           const char* variant_set_utf8, char** out_selected_utf8) {
  if (!stage || !stage->inner || !prim_path_utf8 || !variant_set_utf8 || !out_selected_utf8) {
    set_error("freeusd_stage_get_composed_prim_variant_selection_utf8: null argument");
    return FREEUSD_ERR_INVALID_ARGUMENT;
  }
  *out_selected_utf8 = nullptr;
  if (variant_set_utf8[0] == '\0') {
    set_error("empty variant set name");
    return FREEUSD_ERR_INVALID_ARGUMENT;
  }
  try {
    const freeusd::sdf::Path p = freeusd::sdf::Path::FromString(prim_path_utf8);
    if (p.IsEmpty() || !p.IsPrimPath()) {
      set_error("invalid prim path");
      return FREEUSD_ERR_INVALID_ARGUMENT;
    }
    std::string name;
    if (!stage->inner->GetComposedPrimVariantSelection(p, std::string{variant_set_utf8}, &name)) {
      set_error("no composed variantSelection for set");
      return FREEUSD_ERR_NOT_FOUND;
    }
    char* dup = dup_cstr(name);
    if (!dup) {
      set_error("out of memory");
      return FREEUSD_ERR_INTERNAL;
    }
    *out_selected_utf8 = dup;
    clear_error();
    return FREEUSD_OK;
  } catch (const std::exception& e) {
    set_error(e.what());
    return FREEUSD_ERR_INTERNAL;
  } catch (...) {
    set_error("unknown exception");
    return FREEUSD_ERR_INTERNAL;
  }
}

int freeusd_stage_prim_variant_selection_set_in_any_layer(const FreeusdStage* stage, const char* prim_path_utf8,
                                                          const char* variant_set_utf8) {
  if (!stage || !stage->inner || !prim_path_utf8 || !variant_set_utf8) {
    set_error("freeusd_stage_prim_variant_selection_set_in_any_layer: null argument");
    return FREEUSD_ERR_INVALID_ARGUMENT;
  }
  try {
    const freeusd::sdf::Path p = freeusd::sdf::Path::FromString(prim_path_utf8);
    if (p.IsEmpty() || !p.IsPrimPath()) {
      set_error("invalid prim path");
      return FREEUSD_ERR_INVALID_ARGUMENT;
    }
    const int out =
        stage->inner->PrimVariantSelectionSetInAnyLayer(p, std::string{variant_set_utf8}) ? 1 : 0;
    clear_error();
    return out;
  } catch (const std::exception& e) {
    set_error(e.what());
    return FREEUSD_ERR_INTERNAL;
  } catch (...) {
    set_error("unknown exception");
    return FREEUSD_ERR_INTERNAL;
  }
}

int freeusd_stage_list_composed_prim_variant_selection_sets_utf8(const FreeusdStage* stage, const char* prim_path_utf8,
                                                                  char*** out_strings, size_t* out_count) {
  if (!stage || !stage->inner || !prim_path_utf8 || !out_strings || !out_count) {
    set_error("freeusd_stage_list_composed_prim_variant_selection_sets_utf8: null argument");
    return FREEUSD_ERR_INVALID_ARGUMENT;
  }
  *out_strings = nullptr;
  *out_count = 0;
  try {
    const freeusd::sdf::Path p = freeusd::sdf::Path::FromString(prim_path_utf8);
    if (p.IsEmpty() || !p.IsPrimPath()) {
      set_error("invalid prim path");
      return FREEUSD_ERR_INVALID_ARGUMENT;
    }
    const std::vector<std::string> items = stage->inner->ListComposedPrimVariantSelectionSets(p);
    const int rc = malloc_string_list(items, out_strings, out_count);
    if (rc != FREEUSD_OK) {
      set_error(rc == FREEUSD_ERR_INTERNAL ? "out of memory" : "invalid argument");
      return rc;
    }
    clear_error();
    return FREEUSD_OK;
  } catch (const std::exception& e) {
    set_error(e.what());
    return FREEUSD_ERR_INTERNAL;
  } catch (...) {
    set_error("unknown exception");
    return FREEUSD_ERR_INTERNAL;
  }
}

int freeusd_stage_list_composed_prim_variant_set_names_utf8(const FreeusdStage* stage, const char* prim_path_utf8,
                                                            char*** out_strings, size_t* out_count) {
  if (!stage || !stage->inner || !prim_path_utf8 || !out_strings || !out_count) {
    set_error("freeusd_stage_list_composed_prim_variant_set_names_utf8: null argument");
    return FREEUSD_ERR_INVALID_ARGUMENT;
  }
  *out_strings = nullptr;
  *out_count = 0;
  try {
    const freeusd::sdf::Path p = freeusd::sdf::Path::FromString(prim_path_utf8);
    if (p.IsEmpty() || !p.IsPrimPath()) {
      set_error("invalid prim path");
      return FREEUSD_ERR_INVALID_ARGUMENT;
    }
    const std::vector<std::string> items = stage->inner->ListComposedPrimVariantSetNames(p);
    const int rc = malloc_string_list(items, out_strings, out_count);
    if (rc != FREEUSD_OK) {
      set_error(rc == FREEUSD_ERR_INTERNAL ? "out of memory" : "invalid argument");
      return rc;
    }
    clear_error();
    return FREEUSD_OK;
  } catch (const std::exception& e) {
    set_error(e.what());
    return FREEUSD_ERR_INTERNAL;
  } catch (...) {
    set_error("unknown exception");
    return FREEUSD_ERR_INTERNAL;
  }
}

int freeusd_stage_prim_variant_set_declared_in_any_layer(const FreeusdStage* stage, const char* prim_path_utf8,
                                                         const char* variant_set_name_utf8) {
  if (!stage || !stage->inner || !prim_path_utf8 || !variant_set_name_utf8) {
    set_error("freeusd_stage_prim_variant_set_declared_in_any_layer: null argument");
    return FREEUSD_ERR_INVALID_ARGUMENT;
  }
  try {
    const freeusd::sdf::Path p = freeusd::sdf::Path::FromString(prim_path_utf8);
    if (p.IsEmpty() || !p.IsPrimPath()) {
      set_error("invalid prim path");
      return FREEUSD_ERR_INVALID_ARGUMENT;
    }
    const int out =
        stage->inner->PrimVariantSetDeclaredInAnyLayer(p, std::string{variant_set_name_utf8}) ? 1 : 0;
    clear_error();
    return out;
  } catch (const std::exception& e) {
    set_error(e.what());
    return FREEUSD_ERR_INTERNAL;
  } catch (...) {
    set_error("unknown exception");
    return FREEUSD_ERR_INTERNAL;
  }
}

int freeusd_stage_list_composed_prim_variant_names_utf8(const FreeusdStage* stage, const char* prim_path_utf8,
                                                        const char* variant_set_name_utf8, char*** out_strings,
                                                        size_t* out_count) {
  if (!stage || !stage->inner || !prim_path_utf8 || !variant_set_name_utf8 || !out_strings || !out_count) {
    set_error("freeusd_stage_list_composed_prim_variant_names_utf8: null argument");
    return FREEUSD_ERR_INVALID_ARGUMENT;
  }
  *out_strings = nullptr;
  *out_count = 0;
  if (variant_set_name_utf8[0] == '\0') {
    set_error("empty variant set name");
    return FREEUSD_ERR_INVALID_ARGUMENT;
  }
  try {
    const freeusd::sdf::Path p = freeusd::sdf::Path::FromString(prim_path_utf8);
    if (p.IsEmpty() || !p.IsPrimPath()) {
      set_error("invalid prim path");
      return FREEUSD_ERR_INVALID_ARGUMENT;
    }
    const std::vector<std::string> items =
        stage->inner->GetComposedPrimVariantNames(p, std::string{variant_set_name_utf8});
    if (items.empty()) {
      set_error("variant set not declared on composed stage");
      return FREEUSD_ERR_NOT_FOUND;
    }
    const int rc = malloc_string_list(items, out_strings, out_count);
    if (rc != FREEUSD_OK) {
      set_error(rc == FREEUSD_ERR_INTERNAL ? "out of memory" : "invalid argument");
      return rc;
    }
    clear_error();
    return FREEUSD_OK;
  } catch (const std::exception& e) {
    set_error(e.what());
    return FREEUSD_ERR_INTERNAL;
  } catch (...) {
    set_error("unknown exception");
    return FREEUSD_ERR_INTERNAL;
  }
}

}  // extern "C"
