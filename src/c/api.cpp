#include "freeusd/c/freeusd.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <memory>
#include <string>
#include <vector>

#include "freeusd/io/usda.hpp"
#include "freeusd/pcp/layerStack.hpp"
#include "freeusd/sdf/layer.hpp"
#include "freeusd/sdf/path.hpp"
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

}  // extern "C"
