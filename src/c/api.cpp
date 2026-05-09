#include "freeusd/c/freeusd.h"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <memory>
#include <string>
#include <variant>
#include <vector>

#include "freeusd/gf/vec3d.hpp"
#include "freeusd/io/usda.hpp"
#include "freeusd/pcp/layerStack.hpp"
#include "freeusd/sdf/layer.hpp"
#include "freeusd/sdf/path.hpp"
#include "freeusd/sdf/primReference.hpp"
#include "freeusd/tf/token.hpp"
#include "freeusd/usd/prim.hpp"
#include "freeusd/usd/stage.hpp"
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

}  // extern "C"
