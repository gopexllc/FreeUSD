#include "freeusd/usd/stage.hpp"

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "freeusd/ar/defaultResolver.hpp"
#include "freeusd/ar/pathSecurity.hpp"
#include "freeusd/io/usda.hpp"
#include "freeusd/pcp/compose.hpp"
#include "freeusd/pcp/resolve.hpp"
#include "freeusd/sdf/layer.hpp"
#include "freeusd/sdf/path.hpp"
#include "freeusd/tf/token.hpp"
#include "freeusd/usd/crateFile.hpp"
#include "freeusd/vt/value.hpp"

namespace freeusd::usd {

namespace {

constexpr int kAttrConnectionHopLimit = 64;

bool map_composed_time_to_layer_time(const freeusd::sdf::LayerOffset& offset, double composed_time, double* out_layer_time) {
  if (!out_layer_time) {
    return false;
  }
  if (offset.scale == 0.0) {
    return false;
  }
  *out_layer_time = (composed_time - offset.offset) / offset.scale;
  return true;
}

double map_layer_time_to_composed_time(const freeusd::sdf::LayerOffset& offset, double layer_time) {
  return layer_time * offset.scale + offset.offset;
}

const freeusd::sdf::LayerOffset& layer_offset_for_index(const std::vector<freeusd::sdf::LayerOffset>& offsets, std::size_t index) {
  static const freeusd::sdf::LayerOffset kIdentity{};
  if (index >= offsets.size()) {
    return kIdentity;
  }
  return offsets[index];
}

std::vector<std::pair<freeusd::sdf::Path, freeusd::sdf::Path>> list_composed_relocates(
    const std::vector<std::shared_ptr<freeusd::sdf::Layer>>& compose) {
  std::unordered_set<std::string> seen_from;
  std::vector<std::pair<freeusd::sdf::Path, freeusd::sdf::Path>> acc;
  for (const std::shared_ptr<freeusd::sdf::Layer>& L : compose) {
    if (!L) {
      continue;
    }
    for (const auto& pr : L->ListRelocates()) {
      const std::string& fs = pr.first.GetString();
      if (seen_from.insert(fs).second) {
        acc.emplace_back(pr.first, pr.second);
      }
    }
  }
  std::sort(acc.begin(), acc.end(), [](const std::pair<freeusd::sdf::Path, freeusd::sdf::Path>& a,
                                       const std::pair<freeusd::sdf::Path, freeusd::sdf::Path>& b) {
    return a.first.GetString() < b.first.GetString();
  });
  return acc;
}

std::vector<std::pair<std::string, std::string>> list_composed_prefix_substitutions(
    const std::vector<std::shared_ptr<freeusd::sdf::Layer>>& compose) {
  std::unordered_set<std::string> seen_from;
  std::vector<std::pair<std::string, std::string>> acc;
  for (const std::shared_ptr<freeusd::sdf::Layer>& L : compose) {
    if (!L) {
      continue;
    }
    for (const auto& pr : L->ListPrefixSubstitutions()) {
      if (seen_from.insert(pr.first).second) {
        acc.emplace_back(pr.first, pr.second);
      }
    }
  }
  std::sort(acc.begin(), acc.end(), [](const std::pair<std::string, std::string>& a,
                                       const std::pair<std::string, std::string>& b) { return a.first < b.first; });
  return acc;
}

bool replace_path_prefix(const freeusd::sdf::Path& input, const freeusd::sdf::Path& from, const freeusd::sdf::Path& to,
                         freeusd::sdf::Path* out) {
  if (!out || !from.IsPrimPath() || !to.IsPrimPath()) {
    return false;
  }
  const freeusd::sdf::Path prim_path = input.IsPropertyPath() ? input.GetPrimPath() : input;
  if (!prim_path.IsPrimPath() || !prim_path.HasPrefix(from)) {
    return false;
  }
  const std::string suffix = prim_path.GetString().substr(from.GetString().size());
  const freeusd::sdf::Path mapped_prim = freeusd::sdf::Path::FromString(to.GetString() + suffix);
  if (!mapped_prim.IsPrimPath()) {
    return false;
  }
  if (input.IsPropertyPath()) {
    *out = freeusd::sdf::Path::FromString(mapped_prim.GetString() + "." + input.GetName());
    return out->IsPropertyPath();
  }
  *out = mapped_prim;
  return true;
}

freeusd::sdf::Path relocate_path_best_match(const std::vector<std::pair<freeusd::sdf::Path, freeusd::sdf::Path>>& pairs,
                                            const freeusd::sdf::Path& input, bool reverse) {
  freeusd::sdf::Path best = input;
  std::size_t best_len = 0;
  for (const auto& pr : pairs) {
    const freeusd::sdf::Path& from = reverse ? pr.second : pr.first;
    const freeusd::sdf::Path& to = reverse ? pr.first : pr.second;
    freeusd::sdf::Path candidate;
    if (!replace_path_prefix(input, from, to, &candidate)) {
      continue;
    }
    const std::size_t cur_len = from.GetString().size();
    if (cur_len >= best_len) {
      best = candidate;
      best_len = cur_len;
    }
  }
  return best;
}

freeusd::sdf::Path map_authored_to_composed_path(const std::vector<std::shared_ptr<freeusd::sdf::Layer>>& compose,
                                                 const freeusd::sdf::Path& authored) {
  return relocate_path_best_match(list_composed_relocates(compose), authored, false);
}

freeusd::sdf::Path map_composed_to_authored_path(const std::vector<std::shared_ptr<freeusd::sdf::Layer>>& compose,
                                                 const freeusd::sdf::Path& composed) {
  return relocate_path_best_match(list_composed_relocates(compose), composed, true);
}

std::string apply_composed_prefix_substitutions(const std::vector<std::shared_ptr<freeusd::sdf::Layer>>& compose,
                                                std::string asset_path) {
  std::size_t best_len = 0;
  std::string best_to;
  for (const auto& pr : list_composed_prefix_substitutions(compose)) {
    if (pr.first.empty() || asset_path.rfind(pr.first, 0) != 0) {
      continue;
    }
    if (pr.first.size() >= best_len) {
      best_len = pr.first.size();
      best_to = pr.second;
    }
  }
  if (best_len == 0) {
    return asset_path;
  }
  return best_to + asset_path.substr(best_len);
}

std::vector<freeusd::sdf::Path> map_authored_targets_to_composed(
    const std::vector<std::shared_ptr<freeusd::sdf::Layer>>& compose, std::vector<freeusd::sdf::Path> paths) {
  for (auto& p : paths) {
    p = map_authored_to_composed_path(compose, p);
  }
  return paths;
}

std::vector<freeusd::sdf::PrimReference> apply_composed_prefix_substitutions_to_refs(
    const std::vector<std::shared_ptr<freeusd::sdf::Layer>>& compose, std::vector<freeusd::sdf::PrimReference> refs) {
  for (auto& ref : refs) {
    if (!ref.asset_path.empty()) {
      ref.asset_path = apply_composed_prefix_substitutions(compose, std::move(ref.asset_path));
    }
  }
  return refs;
}

bool ResolveAttributeConnectionStrongestFirst(const std::vector<std::shared_ptr<freeusd::sdf::Layer>>& strongest_first,
                                              const freeusd::sdf::Path& prim_path, const freeusd::tf::Token& name,
                                              freeusd::sdf::Path* target_prop_path) {
  if (!target_prop_path || name.IsEmpty()) {
    return false;
  }
  const freeusd::sdf::Path relocated =
      map_composed_to_authored_path(strongest_first, prim_path);
  for (const freeusd::sdf::Path& candidate : {prim_path, relocated}) {
    for (const std::shared_ptr<freeusd::sdf::Layer>& L : strongest_first) {
      if (!L) {
        continue;
      }
      if (L->HasAttributeConnection(candidate, name)) {
        return L->GetAttributeConnectionTarget(candidate, name, target_prop_path);
      }
    }
    if (candidate == relocated) {
      break;
    }
  }
  return false;
}

std::vector<freeusd::sdf::Path> prim_ancestors_deepest_first(const freeusd::sdf::Path& prim_path) {
  std::vector<freeusd::sdf::Path> out;
  if (!prim_path.IsPrimPath()) {
    return out;
  }
  for (freeusd::sdf::Path cur = prim_path; cur.IsPrimPath(); cur = cur.GetParentPath()) {
    out.push_back(cur);
    if (cur.GetParentPath().IsAbsoluteRootPath()) {
      break;
    }
  }
  return out;
}

bool map_subtree_query_path(const freeusd::sdf::Path& query_path, const freeusd::sdf::Path& source_root,
                            const freeusd::sdf::Path& target_root, freeusd::sdf::Path* out) {
  if (!out || !query_path.IsPrimPath() || !source_root.IsPrimPath()) {
    return false;
  }
  if (query_path != source_root && !query_path.HasPrefix(source_root)) {
    return false;
  }
  const std::string suffix = query_path.GetString().substr(source_root.GetString().size());
  if (target_root.IsAbsoluteRootPath()) {
    *out = suffix.empty() ? target_root : freeusd::sdf::Path::FromString(suffix);
    return out->IsAbsoluteRootPath() || out->IsPrimPath();
  }
  if (!target_root.IsPrimPath()) {
    return false;
  }
  *out = freeusd::sdf::Path::FromString(target_root.GetString() + suffix);
  return out->IsPrimPath();
}

std::string resolve_asset_relative_to_layer_identifier(const std::string& layer_identifier, const std::string& asset_path) {
  if (asset_path.empty()) {
    return {};
  }
  namespace fs = std::filesystem;
  std::string anchor;
  if (!layer_identifier.empty()) {
    const fs::path base(layer_identifier);
    if (base.has_parent_path()) {
      anchor = base.parent_path().string();
    }
  }
  if (anchor.empty()) {
    anchor = fs::current_path().string();
  }
  return freeusd::ar::ResolvePathUnderAnchor(anchor, asset_path);
}

std::shared_ptr<freeusd::usd::Stage> open_asset_stage_from_reference(
    const freeusd::sdf::Layer& source_layer, const std::vector<std::shared_ptr<freeusd::sdf::Layer>>& compose,
    const freeusd::sdf::PrimReference& ref) {
  if (ref.asset_path.empty()) {
    return {};
  }
  const std::string resolved_asset =
      resolve_asset_relative_to_layer_identifier(source_layer.GetIdentifier(),
                                                apply_composed_prefix_substitutions(compose, ref.asset_path));
  if (resolved_asset.empty()) {
    return {};
  }
  return freeusd::usd::Stage::OpenFromRootFile(resolved_asset, freeusd::usd::RootLayerSublayersPolicy::DepthFirst, nullptr);
}

freeusd::sdf::Path reference_target_root(const freeusd::usd::Stage& target_stage, const freeusd::sdf::PrimReference& ref) {
  if (ref.prim_path.has_value() && ref.prim_path->IsPrimPath()) {
    return *ref.prim_path;
  }
  if (target_stage.HasDefaultPrim()) {
    return freeusd::sdf::Path::AbsoluteRootPath().AppendChild(freeusd::tf::Token(target_stage.GetDefaultPrimName()));
  }
  const std::vector<freeusd::usd::Prim> roots = target_stage.GetChildren(freeusd::sdf::Path::AbsoluteRootPath());
  if (!roots.empty()) {
    return roots.front().GetPath();
  }
  return freeusd::sdf::Path::AbsoluteRootPath();
}

/// Variant payloads are embedded inside a synthetic Scope; reject ``}`` that would close it early (USDA injection).
bool variant_payload_stays_within_wrapper(std::string_view body) {
  int depth = 1;
  bool in_double = false;
  bool in_single = false;
  for (std::size_t i = 0; i < body.size(); ++i) {
    const char c = body[i];
    if (!in_single && c == '"' && (i == 0 || body[i - 1] != '\\')) {
      in_double = !in_double;
      continue;
    }
    if (!in_double && c == '\'' && (i == 0 || body[i - 1] != '\\')) {
      in_single = !in_single;
      continue;
    }
    if (in_double || in_single) {
      continue;
    }
    if (c == '{') {
      ++depth;
    } else if (c == '}') {
      --depth;
      if (depth < 1) {
        return false;
      }
    }
  }
  return depth == 1;
}

std::shared_ptr<freeusd::usd::Stage> build_variant_stage_from_payload(const std::string& payload_body) {
  if (!variant_payload_stays_within_wrapper(payload_body)) {
    return {};
  }
  auto layer = freeusd::sdf::Layer::NewAnonymous("__variant__.usda");
  const std::string wrapped = "#usda 1.0\n(\n)\ndef Scope \"__VariantRoot\"\n{\n" + payload_body + "\n}\n";
  const auto parsed = freeusd::io::usda::LoadFromString(wrapped, layer);
  if (!parsed.ok) {
    return {};
  }
  return freeusd::usd::Stage::AttachRootLayer(std::move(layer));
}

struct ActiveQueryGuard {
  std::unordered_set<std::string>* set = nullptr;
  std::string key;
  ~ActiveQueryGuard() {
    if (set) {
      set->erase(key);
    }
  }
};

bool visit_composition_arc_prim_paths(
    const freeusd::usd::Stage& self, const std::vector<std::shared_ptr<freeusd::sdf::Layer>>& compose,
    const freeusd::sdf::Path& authored,
    const std::function<bool(const freeusd::usd::Stage& stage, const freeusd::sdf::Path& prim_path)>& visitor) {
  if (!visitor) {
    return false;
  }
  static thread_local std::unordered_set<std::string> active_queries;
  const std::string active_key =
      std::to_string(reinterpret_cast<std::uintptr_t>(&self)) + "|primMetaArc|" + authored.GetString();
  if (!active_queries.insert(active_key).second) {
    return false;
  }
  ActiveQueryGuard active_guard{&active_queries, active_key};
  for (const std::shared_ptr<freeusd::sdf::Layer>& layer : compose) {
    if (!layer) {
      continue;
    }
    for (const freeusd::sdf::Path& ancestor : prim_ancestors_deepest_first(authored)) {
      freeusd::sdf::Path mapped_query;
      const auto try_reference_list = [&](const std::vector<freeusd::sdf::PrimReference>& refs) -> bool {
        for (const freeusd::sdf::PrimReference& ref : refs) {
          std::shared_ptr<freeusd::usd::Stage> target_stage = open_asset_stage_from_reference(*layer, compose, ref);
          if (!target_stage) {
            continue;
          }
          if (!map_subtree_query_path(authored, ancestor, reference_target_root(*target_stage, ref), &mapped_query)) {
            continue;
          }
          if (visitor(*target_stage, mapped_query)) {
            return true;
          }
        }
        return false;
      };
      const auto try_internal_arcs = [&](const std::vector<freeusd::sdf::Path>& targets) -> bool {
        for (const freeusd::sdf::Path& target_root : targets) {
          if (!map_subtree_query_path(authored, ancestor, target_root, &mapped_query)) {
            continue;
          }
          const freeusd::sdf::Path mapped_composed = map_authored_to_composed_path(compose, mapped_query);
          if (visitor(self, mapped_composed)) {
            return true;
          }
        }
        return false;
      };
      if (try_reference_list(layer->ListPrimReferences(ancestor)) ||
          try_reference_list(layer->ListPrimPayloads(ancestor)) ||
          try_internal_arcs(layer->ListPrimInherits(ancestor)) ||
          try_internal_arcs(layer->ListPrimSpecializes(ancestor))) {
        return true;
      }
    }
  }
  return false;
}

}  // namespace

Stage::Stage(std::vector<std::shared_ptr<freeusd::sdf::Layer>> compose,
             std::vector<freeusd::sdf::LayerOffset> compose_offsets)
    : compose_(std::move(compose)),
      compose_offsets_(std::move(compose_offsets)),
      resolver_(std::make_unique<freeusd::ar::DefaultResolver>()) {}

std::shared_ptr<Stage> Stage::AttachRootLayer(std::shared_ptr<freeusd::sdf::Layer> root) {
  if (!root) {
    return {};
  }
  std::vector<std::shared_ptr<freeusd::sdf::Layer>> one;
  one.push_back(std::move(root));
  return std::shared_ptr<Stage>(new Stage(std::move(one), std::vector<freeusd::sdf::LayerOffset>(1)));
}

std::shared_ptr<Stage> Stage::AttachLayerStack(freeusd::pcp::LayerStack stack) {
  std::vector<std::shared_ptr<freeusd::sdf::Layer>> lys;
  std::vector<freeusd::sdf::LayerOffset> offsets;
  lys.reserve(stack.GetEntries().size());
  offsets.reserve(stack.GetEntries().size());
  for (const freeusd::pcp::LayerStackEntry& entry : stack.GetEntries()) {
    lys.push_back(entry.layer);
    offsets.push_back(entry.offset);
  }
  if (lys.empty()) {
    return {};
  }
  return std::shared_ptr<Stage>(new Stage(std::move(lys), std::move(offsets)));
}

std::shared_ptr<Stage> Stage::OpenFromRootFile(const std::string& layer_path, RootLayerSublayersPolicy sublayers,
                                               std::string* err_detail) {
  if (err_detail) {
    err_detail->clear();
  }
  namespace fs = std::filesystem;
  std::error_code ec;
  const fs::path raw{layer_path};
  if (!fs::exists(raw, ec) || ec) {
    if (err_detail) {
      *err_detail = "layer file not found: " + layer_path;
    }
    return {};
  }
  const fs::path lp = fs::weakly_canonical(raw, ec);
  if (ec) {
    if (err_detail) {
      *err_detail = ec.message();
    }
    return {};
  }
  std::string anchor;
  if (lp.has_parent_path()) {
    anchor = lp.parent_path().string();
  } else {
    anchor = fs::current_path().string();
  }

  auto root = freeusd::sdf::Layer::NewAnonymous(lp.filename().string());
  root->SetIdentifier(lp.string());
  freeusd::io::usda::ParseResult pr{};
  std::string embedded_usda;
  const freeusd::usd::crate::UsdFileKind kind = freeusd::usd::crate::DetectUsdFileKindFromPath(lp.string(), nullptr);
  if (kind == freeusd::usd::crate::UsdFileKind::UsdcCrate) {
    std::string crate_err;
    if (!freeusd::usd::crate::ReadUsdCrateUsdaSectionFromPath(lp.string(), embedded_usda, 8u * 1024u * 1024u, &crate_err)) {
      if (err_detail) {
        *err_detail = crate_err.empty() ? "USDC stage opening requires an embedded USDA section in this build"
                                        : crate_err;
      }
      return {};
    }
    pr = freeusd::io::usda::LoadFromString(embedded_usda, root);
  } else {
    pr = freeusd::io::usda::LoadFromFile(lp.string(), root);
  }
  if (!pr.ok) {
    if (err_detail) {
      *err_detail = "USDA parse error: " + pr.message + " (line " + std::to_string(pr.line) + ")";
    }
    return {};
  }

  if (sublayers == RootLayerSublayersPolicy::None) {
    return AttachRootLayer(std::move(root));
  }

  const std::string anchor_canon = freeusd::ar::CanonicalizeFilesystemPath(anchor);
  freeusd::ar::DefaultResolver resolver(anchor_canon);
  auto resolve = [resolver, anchor_canon](const std::string& authored) mutable -> std::shared_ptr<freeusd::sdf::Layer> {
    const std::string abs = resolver.Resolve(std::string_view{authored});
    if (abs.empty() || anchor_canon.empty() ||
        !freeusd::ar::PathIsWithinRootDirectory(abs, anchor_canon)) {
      return {};
    }
    std::error_code e2;
    const fs::path ap{abs};
    if (!fs::exists(ap, e2) || e2) {
      return {};
    }
    const fs::path canon = fs::weakly_canonical(ap, e2);
    if (e2 || !freeusd::ar::PathIsWithinRootDirectory(canon.string(), anchor_canon)) {
      return {};
    }
    auto L = freeusd::sdf::Layer::NewAnonymous(canon.filename().string());
    L->SetIdentifier(canon.string());
    const auto pr2 = freeusd::io::usda::LoadFromFile(canon.string(), L);
    if (!pr2.ok) {
      return {};
    }
    return L;
  };

  if (sublayers == RootLayerSublayersPolicy::Shallow) {
    return AttachLayerStack(freeusd::pcp::ComposeSublayers(root, resolve));
  }
  if (sublayers == RootLayerSublayersPolicy::DepthFirst) {
    return AttachLayerStack(freeusd::pcp::ComposeSublayersDepthFirst(root, resolve));
  }
  return AttachRootLayer(std::move(root));
}

bool Stage::ReadFieldAtEvaluatedTime(const freeusd::sdf::Path& prim_path, const freeusd::tf::Token& name, double time,
                                     freeusd::vt::Value* out) const {
  if (!out || name.IsEmpty()) {
    return false;
  }
  const freeusd::sdf::Path relocated_authored = map_composed_to_authored_path(compose_, prim_path);
  if (map_authored_to_composed_path(compose_, prim_path) != prim_path &&
      map_authored_to_composed_path(compose_, relocated_authored) != prim_path) {
    return false;
  }
  freeusd::sdf::Path cur_prim = prim_path;
  freeusd::tf::Token cur_name = name;
  std::unordered_set<std::string> seen;
  for (int hops = 0; hops <= kAttrConnectionHopLimit; ++hops) {
    const std::string step = cur_prim.GetString() + '\x1f' + cur_name.GetText();
    if (!seen.insert(step).second) {
      return false;
    }
    freeusd::sdf::Path conn_to;
    if (!ResolveAttributeConnectionStrongestFirst(compose_, cur_prim, cur_name, &conn_to)) {
      const auto try_field_on_prim = [&](const freeusd::sdf::Path& query_prim) -> bool {
        for (std::size_t i = 0; i < compose_.size(); ++i) {
          const std::shared_ptr<freeusd::sdf::Layer>& layer = compose_[i];
          if (!layer) {
            continue;
          }
          double layer_time = time;
          if (!map_composed_time_to_layer_time(layer_offset_for_index(compose_offsets_, i), time, &layer_time)) {
            continue;
          }
          if (layer->GetFieldAtTime(query_prim, cur_name, layer_time, out)) {
            return true;
          }
        }
        return false;
      };
      if (try_field_on_prim(cur_prim)) {
        return true;
      }
      if (relocated_authored != cur_prim && try_field_on_prim(relocated_authored)) {
        return true;
      }
      static thread_local std::unordered_set<std::string> active_queries;
      const std::string active_key = std::to_string(reinterpret_cast<std::uintptr_t>(this)) + "|" + cur_prim.GetString() +
                                     "|" + cur_name.GetText() + "|" + std::to_string(time);
      if (!active_queries.insert(active_key).second) {
        return false;
      }
      ActiveQueryGuard active_guard{&active_queries, active_key};
      const freeusd::sdf::Path variant_root = freeusd::sdf::Path::FromString("/__VariantRoot");
      freeusd::sdf::Path arc_query = cur_prim;
      {
        const freeusd::sdf::Path mapped = map_composed_to_authored_path(compose_, cur_prim);
        if (map_authored_to_composed_path(compose_, mapped) == cur_prim) {
          arc_query = mapped;
        }
      }
      for (const std::shared_ptr<freeusd::sdf::Layer>& layer : compose_) {
        if (!layer) {
          continue;
        }
        for (const freeusd::sdf::Path& ancestor : prim_ancestors_deepest_first(arc_query)) {
          freeusd::sdf::Path mapped_query;
          const auto try_reference_list = [&](const std::vector<freeusd::sdf::PrimReference>& refs) -> bool {
            for (const freeusd::sdf::PrimReference& ref : refs) {
              std::shared_ptr<Stage> target_stage = open_asset_stage_from_reference(*layer, compose_, ref);
              if (!target_stage) {
                continue;
              }
              if (!map_subtree_query_path(arc_query, ancestor, reference_target_root(*target_stage, ref), &mapped_query)) {
                continue;
              }
              freeusd::vt::Value tmp;
              if (target_stage->ReadFieldAtEvaluatedTime(mapped_query, cur_name, time, &tmp)) {
                *out = std::move(tmp);
                return true;
              }
            }
            return false;
          };
          const auto try_internal_arcs = [&](const std::vector<freeusd::sdf::Path>& targets) -> bool {
            for (const freeusd::sdf::Path& target_root : targets) {
              if (!map_subtree_query_path(arc_query, ancestor, target_root, &mapped_query)) {
                continue;
              }
              freeusd::vt::Value tmp;
              if (ReadFieldAtEvaluatedTime(map_authored_to_composed_path(compose_, mapped_query), cur_name, time, &tmp)) {
                *out = std::move(tmp);
                return true;
              }
            }
            return false;
          };
          for (const std::string& set_name : layer->ListPrimVariantSetNames(ancestor)) {
            std::string selected;
            if (!GetComposedPrimVariantSelection(map_authored_to_composed_path(compose_, ancestor), set_name, &selected)) {
              continue;
            }
            std::string payload_body;
            if (!layer->GetPrimVariantPayload(ancestor, set_name, selected, &payload_body)) {
              continue;
            }
            std::shared_ptr<Stage> variant_stage = build_variant_stage_from_payload(payload_body);
            if (!variant_stage || !map_subtree_query_path(arc_query, ancestor, variant_root, &mapped_query)) {
              continue;
            }
            freeusd::vt::Value tmp;
            if (variant_stage->ReadFieldAtEvaluatedTime(mapped_query, cur_name, time, &tmp)) {
              *out = std::move(tmp);
              return true;
            }
          }
          if (try_reference_list(layer->ListPrimReferences(ancestor))) {
            return true;
          }
          if (try_reference_list(layer->ListPrimPayloads(ancestor))) {
            return true;
          }
          if (try_internal_arcs(layer->ListPrimInherits(ancestor))) {
            return true;
          }
          if (try_internal_arcs(layer->ListPrimSpecializes(ancestor))) {
            return true;
          }
        }
      }
      return false;
    }
    if (!conn_to.IsPropertyPath()) {
      return false;
    }
    cur_prim = conn_to.GetPrimPath();
    cur_name = freeusd::tf::Token{conn_to.GetName()};
    if (cur_name.IsEmpty()) {
      return false;
    }
  }
  return false;
}

bool Stage::HasFieldOpinion(const freeusd::sdf::Path& prim_path, const freeusd::tf::Token& name) const {
  if (name.IsEmpty()) {
    return false;
  }
  freeusd::vt::Value tmp;
  if (ReadFieldAtEvaluatedTime(prim_path, name, 1.0, &tmp)) {
    return true;
  }
  const freeusd::sdf::Path authored = map_composed_to_authored_path(compose_, prim_path);
  if (map_authored_to_composed_path(compose_, authored) != prim_path) {
    return false;
  }
  for (const std::shared_ptr<freeusd::sdf::Layer>& L : compose_) {
    if (L && L->HasAttributeConnection(authored, name)) {
      return true;
    }
  }
  return false;
}

bool Stage::HasAttributeConnection(const freeusd::sdf::Path& prim_path, const freeusd::tf::Token& attr_name) const {
  if (attr_name.IsEmpty()) {
    return false;
  }
  const freeusd::sdf::Path authored = map_composed_to_authored_path(compose_, prim_path);
  if (map_authored_to_composed_path(compose_, authored) != prim_path) {
    return false;
  }
  for (const std::shared_ptr<freeusd::sdf::Layer>& L : compose_) {
    if (L && L->HasAttributeConnection(authored, attr_name)) {
      return true;
    }
  }
  return false;
}

bool Stage::GetComposedAttributeConnectionTarget(const freeusd::sdf::Path& prim_path,
                                                 const freeusd::tf::Token& attr_name,
                                                 freeusd::sdf::Path* out_target) const {
  if (!out_target || attr_name.IsEmpty()) {
    return false;
  }
  const freeusd::sdf::Path authored = map_composed_to_authored_path(compose_, prim_path);
  if (map_authored_to_composed_path(compose_, authored) != prim_path) {
    return false;
  }
  freeusd::sdf::Path target;
  if (!ResolveAttributeConnectionStrongestFirst(compose_, authored, attr_name, &target)) {
    return false;
  }
  *out_target = map_authored_to_composed_path(compose_, target);
  return true;
}

bool Stage::PrimPathInUse(const freeusd::sdf::Path& path) const {
  if (!path.IsPrimPath()) {
    return false;
  }
  const freeusd::sdf::Path authored = map_composed_to_authored_path(compose_, path);
  if (map_authored_to_composed_path(compose_, authored) != path) {
    return false;
  }
  for (const std::shared_ptr<freeusd::sdf::Layer>& L : compose_) {
    if (!L) {
      continue;
    }
    for (const auto& q : L->ListPrimPaths()) {
      if (q == authored) {
        return true;
      }
    }
  }
  static thread_local std::unordered_set<std::string> active_queries;
  const std::string active_key =
      std::to_string(reinterpret_cast<std::uintptr_t>(this)) + "|prim|" + authored.GetString();
  if (!active_queries.insert(active_key).second) {
    return false;
  }
  ActiveQueryGuard active_guard{&active_queries, active_key};
  const freeusd::sdf::Path variant_root = freeusd::sdf::Path::FromString("/__VariantRoot");
  for (const std::shared_ptr<freeusd::sdf::Layer>& layer : compose_) {
    if (!layer) {
      continue;
    }
    for (const freeusd::sdf::Path& ancestor : prim_ancestors_deepest_first(authored)) {
      freeusd::sdf::Path mapped_query;
      for (const std::string& set_name : layer->ListPrimVariantSetNames(ancestor)) {
        std::string selected;
        std::string payload_body;
        if (!GetComposedPrimVariantSelection(map_authored_to_composed_path(compose_, ancestor), set_name, &selected) ||
            !layer->GetPrimVariantPayload(ancestor, set_name, selected, &payload_body) ||
            !map_subtree_query_path(authored, ancestor, variant_root, &mapped_query)) {
          continue;
        }
        std::shared_ptr<Stage> variant_stage = build_variant_stage_from_payload(payload_body);
        if (variant_stage && variant_stage->PrimPathInUse(mapped_query)) {
          return true;
        }
      }
      const auto try_reference_list = [&](const std::vector<freeusd::sdf::PrimReference>& refs) -> bool {
        for (const freeusd::sdf::PrimReference& ref : refs) {
          std::shared_ptr<Stage> target_stage = open_asset_stage_from_reference(*layer, compose_, ref);
          if (!target_stage || !map_subtree_query_path(authored, ancestor, reference_target_root(*target_stage, ref), &mapped_query)) {
            continue;
          }
          if (target_stage->PrimPathInUse(mapped_query)) {
            return true;
          }
        }
        return false;
      };
      if (try_reference_list(layer->ListPrimReferences(ancestor)) || try_reference_list(layer->ListPrimPayloads(ancestor))) {
        return true;
      }
      const auto try_internal_arcs = [&](const std::vector<freeusd::sdf::Path>& targets) -> bool {
        for (const freeusd::sdf::Path& target_root : targets) {
          if (!map_subtree_query_path(authored, ancestor, target_root, &mapped_query)) {
            continue;
          }
          if (PrimPathInUse(map_authored_to_composed_path(compose_, mapped_query))) {
            return true;
          }
        }
        return false;
      };
      if (try_internal_arcs(layer->ListPrimInherits(ancestor)) || try_internal_arcs(layer->ListPrimSpecializes(ancestor))) {
        return true;
      }
    }
  }
  return false;
}

bool Stage::ReadRelationship(const freeusd::sdf::Path& prim_path, const freeusd::tf::Token& rel_name,
                             std::vector<freeusd::sdf::Path>* out_targets) const {
  if (!out_targets || rel_name.IsEmpty()) {
    return false;
  }
  out_targets->clear();
  const freeusd::sdf::Path authored = map_composed_to_authored_path(compose_, prim_path);
  if (map_authored_to_composed_path(compose_, authored) != prim_path) {
    return false;
  }
  bool any = false;
  for (const std::shared_ptr<freeusd::sdf::Layer>& L : compose_) {
    if (!L || !L->HasRelationship(authored, rel_name)) {
      continue;
    }
    const std::vector<freeusd::sdf::Path> part = map_authored_targets_to_composed(compose_, L->GetRelationshipTargets(authored, rel_name));
    out_targets->insert(out_targets->end(), part.begin(), part.end());
    any = true;
  }
  static thread_local std::unordered_set<std::string> active_queries;
  const std::string active_key = std::to_string(reinterpret_cast<std::uintptr_t>(this)) + "|rel|" + authored.GetString() +
                                 "|" + rel_name.GetText();
  if (!active_queries.insert(active_key).second) {
    return any;
  }
  ActiveQueryGuard active_guard{&active_queries, active_key};
  const freeusd::sdf::Path variant_root = freeusd::sdf::Path::FromString("/__VariantRoot");
  for (const std::shared_ptr<freeusd::sdf::Layer>& layer : compose_) {
    if (!layer) {
      continue;
    }
    for (const freeusd::sdf::Path& ancestor : prim_ancestors_deepest_first(authored)) {
      freeusd::sdf::Path mapped_query;
      for (const std::string& set_name : layer->ListPrimVariantSetNames(ancestor)) {
        std::string selected;
        std::string payload_body;
        if (!GetComposedPrimVariantSelection(map_authored_to_composed_path(compose_, ancestor), set_name, &selected) ||
            !layer->GetPrimVariantPayload(ancestor, set_name, selected, &payload_body) ||
            !map_subtree_query_path(authored, ancestor, variant_root, &mapped_query)) {
          continue;
        }
        std::shared_ptr<Stage> variant_stage = build_variant_stage_from_payload(payload_body);
        if (!variant_stage) {
          continue;
        }
        std::vector<freeusd::sdf::Path> part;
        if (variant_stage->ReadRelationship(mapped_query, rel_name, &part)) {
          out_targets->insert(out_targets->end(), part.begin(), part.end());
          any = true;
        }
      }
      const auto pull_reference_targets = [&](const std::vector<freeusd::sdf::PrimReference>& refs) {
        for (const freeusd::sdf::PrimReference& ref : refs) {
          std::shared_ptr<Stage> target_stage = open_asset_stage_from_reference(*layer, compose_, ref);
          if (!target_stage || !map_subtree_query_path(authored, ancestor, reference_target_root(*target_stage, ref), &mapped_query)) {
            continue;
          }
          std::vector<freeusd::sdf::Path> part;
          if (target_stage->ReadRelationship(mapped_query, rel_name, &part)) {
            out_targets->insert(out_targets->end(), part.begin(), part.end());
            any = true;
          }
        }
      };
      pull_reference_targets(layer->ListPrimReferences(ancestor));
      pull_reference_targets(layer->ListPrimPayloads(ancestor));
      const auto pull_internal_targets = [&](const std::vector<freeusd::sdf::Path>& targets) {
        for (const freeusd::sdf::Path& target_root : targets) {
          if (!map_subtree_query_path(authored, ancestor, target_root, &mapped_query)) {
            continue;
          }
          std::vector<freeusd::sdf::Path> part;
          if (ReadRelationship(map_authored_to_composed_path(compose_, mapped_query), rel_name, &part)) {
            out_targets->insert(out_targets->end(), part.begin(), part.end());
            any = true;
          }
        }
      };
      pull_internal_targets(layer->ListPrimInherits(ancestor));
      pull_internal_targets(layer->ListPrimSpecializes(ancestor));
    }
  }
  return any;
}

bool Stage::HasRelationship(const freeusd::sdf::Path& prim_path, const freeusd::tf::Token& rel_name) const {
  std::vector<freeusd::sdf::Path> targets;
  return ReadRelationship(prim_path, rel_name, &targets);
}

std::vector<freeusd::sdf::PrimReference> Stage::ReadPrimReferences(const freeusd::sdf::Path& prim_path) const {
  std::vector<freeusd::sdf::PrimReference> out;
  const freeusd::sdf::Path authored = map_composed_to_authored_path(compose_, prim_path);
  if (map_authored_to_composed_path(compose_, authored) != prim_path) {
    return out;
  }
  for (const std::shared_ptr<freeusd::sdf::Layer>& L : compose_) {
    if (!L) {
      continue;
    }
    const std::vector<freeusd::sdf::PrimReference> part =
        apply_composed_prefix_substitutions_to_refs(compose_, L->ListPrimReferences(authored));
    out.insert(out.end(), part.begin(), part.end());
  }
  return out;
}

bool Stage::HasPrimReferences(const freeusd::sdf::Path& prim_path) const {
  return !ReadPrimReferences(prim_path).empty();
}

std::vector<freeusd::sdf::Path> Stage::ReadPrimInherits(const freeusd::sdf::Path& prim_path) const {
  std::vector<freeusd::sdf::Path> out;
  const freeusd::sdf::Path authored = map_composed_to_authored_path(compose_, prim_path);
  if (map_authored_to_composed_path(compose_, authored) != prim_path) {
    return out;
  }
  for (const std::shared_ptr<freeusd::sdf::Layer>& L : compose_) {
    if (!L) {
      continue;
    }
    const std::vector<freeusd::sdf::Path> part = map_authored_targets_to_composed(compose_, L->ListPrimInherits(authored));
    out.insert(out.end(), part.begin(), part.end());
  }
  return out;
}

bool Stage::HasPrimInherits(const freeusd::sdf::Path& prim_path) const {
  return !ReadPrimInherits(prim_path).empty();
}

std::vector<freeusd::sdf::Path> Stage::ReadPrimSpecializes(const freeusd::sdf::Path& prim_path) const {
  std::vector<freeusd::sdf::Path> out;
  const freeusd::sdf::Path authored = map_composed_to_authored_path(compose_, prim_path);
  if (map_authored_to_composed_path(compose_, authored) != prim_path) {
    return out;
  }
  for (const std::shared_ptr<freeusd::sdf::Layer>& L : compose_) {
    if (!L) {
      continue;
    }
    const std::vector<freeusd::sdf::Path> part =
        map_authored_targets_to_composed(compose_, L->ListPrimSpecializes(authored));
    out.insert(out.end(), part.begin(), part.end());
  }
  return out;
}

bool Stage::HasPrimSpecializes(const freeusd::sdf::Path& prim_path) const {
  return !ReadPrimSpecializes(prim_path).empty();
}

std::vector<freeusd::sdf::PrimReference> Stage::ReadPrimPayloads(const freeusd::sdf::Path& prim_path) const {
  std::vector<freeusd::sdf::PrimReference> out;
  const freeusd::sdf::Path authored = map_composed_to_authored_path(compose_, prim_path);
  if (map_authored_to_composed_path(compose_, authored) != prim_path) {
    return out;
  }
  for (const std::shared_ptr<freeusd::sdf::Layer>& L : compose_) {
    if (!L) {
      continue;
    }
    const std::vector<freeusd::sdf::PrimReference> part =
        apply_composed_prefix_substitutions_to_refs(compose_, L->ListPrimPayloads(authored));
    out.insert(out.end(), part.begin(), part.end());
  }
  return out;
}

bool Stage::HasPrimPayloads(const freeusd::sdf::Path& prim_path) const {
  return !ReadPrimPayloads(prim_path).empty();
}

freeusd::tf::Token Stage::ResolvePrimKind(const freeusd::sdf::Path& prim_path) const {
  const freeusd::sdf::Path authored = map_composed_to_authored_path(compose_, prim_path);
  if (map_authored_to_composed_path(compose_, authored) != prim_path) {
    return {};
  }
  for (const std::shared_ptr<freeusd::sdf::Layer>& L : compose_) {
    if (!L) {
      continue;
    }
    if (L->HasPrimKind(authored)) {
      return L->GetPrimKind(authored);
    }
    freeusd::vt::Value kv;
    if (L->GetField(authored, freeusd::tf::Token{"kind"}, &kv)) {
      freeusd::tf::Token t;
      if (kv.GetToken(&t) && !t.IsEmpty()) {
        return t;
      }
      std::string s;
      if (kv.GetString(&s) && !s.empty()) {
        return freeusd::tf::Token{s};
      }
    }
  }
  freeusd::tf::Token arc_kind;
  visit_composition_arc_prim_paths(*this, compose_, authored, [&](const Stage& stage, const freeusd::sdf::Path& p) {
    const freeusd::tf::Token k = stage.ResolvePrimKind(p);
    if (!k.IsEmpty()) {
      arc_kind = k;
      return true;
    }
    return false;
  });
  return arc_kind;
}

bool Stage::ResolveHasPrimKind(const freeusd::sdf::Path& prim_path) const {
  const freeusd::sdf::Path authored = map_composed_to_authored_path(compose_, prim_path);
  if (map_authored_to_composed_path(compose_, authored) != prim_path) {
    return false;
  }
  for (const std::shared_ptr<freeusd::sdf::Layer>& L : compose_) {
    if (!L) {
      continue;
    }
    if (L->HasPrimKind(authored)) {
      return true;
    }
    freeusd::vt::Value kv;
    if (L->GetField(authored, freeusd::tf::Token{"kind"}, &kv)) {
      freeusd::tf::Token t;
      if (kv.GetToken(&t) && !t.IsEmpty()) {
        return true;
      }
      std::string s;
      if (kv.GetString(&s) && !s.empty()) {
        return true;
      }
    }
  }
  return visit_composition_arc_prim_paths(*this, compose_, authored, [&](const Stage& stage, const freeusd::sdf::Path& p) {
    return stage.ResolveHasPrimKind(p);
  });
}

freeusd::sdf::Layer::PrimSpecifierKind Stage::ResolvePrimSpecifierKind(const freeusd::sdf::Path& prim_path) const {
  const freeusd::sdf::Path authored = map_composed_to_authored_path(compose_, prim_path);
  if (map_authored_to_composed_path(compose_, authored) != prim_path) {
    return freeusd::sdf::Layer::PrimSpecifierKind::Default;
  }
  for (const std::shared_ptr<freeusd::sdf::Layer>& L : compose_) {
    if (L && L->HasPrimSpecifierOpinion(authored)) {
      return L->GetPrimSpecifier(authored);
    }
  }
  freeusd::sdf::Layer::PrimSpecifierKind inherited = freeusd::sdf::Layer::PrimSpecifierKind::Default;
  VisitInternalArcMappedPrimPaths(authored, [&](const freeusd::sdf::Path& mapped_composed) {
    const freeusd::sdf::Layer::PrimSpecifierKind kind = ResolvePrimSpecifierKind(mapped_composed);
    if (kind != freeusd::sdf::Layer::PrimSpecifierKind::Default) {
      inherited = kind;
      return true;
    }
    return false;
  });
  return inherited;
}

bool Stage::ResolvePrimActive(const freeusd::sdf::Path& prim_path) const {
  const freeusd::sdf::Path authored = map_composed_to_authored_path(compose_, prim_path);
  if (map_authored_to_composed_path(compose_, authored) != prim_path) {
    return true;
  }
  for (const std::shared_ptr<freeusd::sdf::Layer>& L : compose_) {
    if (L && L->HasPrimActiveOpinion(authored)) {
      return L->IsPrimActive(authored);
    }
  }
  bool arc_active = true;
  if (visit_composition_arc_prim_paths(*this, compose_, authored, [&](const Stage& stage, const freeusd::sdf::Path& p) {
        if (stage.ResolveHasPrimActiveOpinion(p)) {
          arc_active = stage.ResolvePrimActive(p);
          return true;
        }
        return false;
      })) {
    return arc_active;
  }
  return true;
}

bool Stage::ResolveHasPrimActiveOpinion(const freeusd::sdf::Path& prim_path) const {
  const freeusd::sdf::Path authored = map_composed_to_authored_path(compose_, prim_path);
  if (map_authored_to_composed_path(compose_, authored) != prim_path) {
    return false;
  }
  for (const std::shared_ptr<freeusd::sdf::Layer>& L : compose_) {
    if (L && L->HasPrimActiveOpinion(authored)) {
      return true;
    }
  }
  return visit_composition_arc_prim_paths(*this, compose_, authored, [&](const Stage& stage, const freeusd::sdf::Path& p) {
    return stage.ResolveHasPrimActiveOpinion(p);
  });
}

bool Stage::VisitInternalArcMappedPrimPaths(
    const freeusd::sdf::Path& authored,
    const std::function<bool(const freeusd::sdf::Path& mapped_composed)>& visitor) const {
  if (!visitor) {
    return false;
  }
  static thread_local std::unordered_set<std::string> active_queries;
  const std::string active_key =
      std::to_string(reinterpret_cast<std::uintptr_t>(this)) + "|customDataArc|" + authored.GetString();
  if (!active_queries.insert(active_key).second) {
    return false;
  }
  ActiveQueryGuard active_guard{&active_queries, active_key};
  for (const std::shared_ptr<freeusd::sdf::Layer>& layer : compose_) {
    if (!layer) {
      continue;
    }
    for (const freeusd::sdf::Path& ancestor : prim_ancestors_deepest_first(authored)) {
      freeusd::sdf::Path mapped_query;
      const auto try_internal_arcs = [&](const std::vector<freeusd::sdf::Path>& targets) -> bool {
        for (const freeusd::sdf::Path& target_root : targets) {
          if (!map_subtree_query_path(authored, ancestor, target_root, &mapped_query)) {
            continue;
          }
          const freeusd::sdf::Path mapped_composed = map_authored_to_composed_path(compose_, mapped_query);
          if (visitor(mapped_composed)) {
            return true;
          }
        }
        return false;
      };
      if (try_internal_arcs(layer->ListPrimInherits(ancestor)) ||
          try_internal_arcs(layer->ListPrimSpecializes(ancestor))) {
        return true;
      }
    }
  }
  return false;
}

bool Stage::GetComposedPrimCustomData(const freeusd::sdf::Path& prim_path, const std::string& key,
                                      freeusd::vt::Value* out) const {
  if (!out || key.empty()) {
    return false;
  }
  const freeusd::sdf::Path authored = map_composed_to_authored_path(compose_, prim_path);
  if (map_authored_to_composed_path(compose_, authored) != prim_path) {
    return false;
  }
  for (const std::shared_ptr<freeusd::sdf::Layer>& L : compose_) {
    if (L && L->HasPrimCustomDataKey(authored, key)) {
      return L->GetPrimCustomDataEntry(authored, key, out);
    }
  }
  freeusd::vt::Value tmp;
  const bool found = VisitInternalArcMappedPrimPaths(authored, [&](const freeusd::sdf::Path& mapped_composed) {
    return GetComposedPrimCustomData(mapped_composed, key, &tmp);
  });
  if (found) {
    *out = std::move(tmp);
  }
  return found;
}

bool Stage::PrimCustomDataKeyInAnyLayer(const freeusd::sdf::Path& prim_path, const std::string& key) const {
  if (key.empty()) {
    return false;
  }
  const freeusd::sdf::Path authored = map_composed_to_authored_path(compose_, prim_path);
  if (map_authored_to_composed_path(compose_, authored) != prim_path) {
    return false;
  }
  for (const std::shared_ptr<freeusd::sdf::Layer>& L : compose_) {
    if (L && L->HasPrimCustomDataKey(authored, key)) {
      return true;
    }
  }
  return VisitInternalArcMappedPrimPaths(authored, [&](const freeusd::sdf::Path& mapped_composed) {
    return PrimCustomDataKeyInAnyLayer(mapped_composed, key);
  });
}

std::vector<std::string> Stage::ListComposedPrimCustomDataKeys(const freeusd::sdf::Path& prim_path) const {
  std::set<std::string> keys;
  const freeusd::sdf::Path authored = map_composed_to_authored_path(compose_, prim_path);
  if (map_authored_to_composed_path(compose_, authored) != prim_path) {
    return {};
  }
  for (const std::shared_ptr<freeusd::sdf::Layer>& L : compose_) {
    if (!L) {
      continue;
    }
    for (const std::string& k : L->ListPrimCustomDataKeys(authored)) {
      keys.insert(k);
    }
  }
  VisitInternalArcMappedPrimPaths(authored, [&](const freeusd::sdf::Path& mapped_composed) {
    for (const std::string& k : ListComposedPrimCustomDataKeys(mapped_composed)) {
      keys.insert(k);
    }
    return false;
  });
  return std::vector<std::string>(keys.begin(), keys.end());
}

bool Stage::GetComposedCustomLayerData(const std::string& key, freeusd::vt::Value* out) const {
  if (!out || key.empty()) {
    return false;
  }
  for (const std::shared_ptr<freeusd::sdf::Layer>& L : compose_) {
    if (L && L->HasCustomLayerDataKey(key)) {
      return L->GetCustomLayerDataEntry(key, out);
    }
  }
  return false;
}

bool Stage::CustomLayerDataKeyInAnyLayer(const std::string& key) const {
  if (key.empty()) {
    return false;
  }
  for (const std::shared_ptr<freeusd::sdf::Layer>& L : compose_) {
    if (L && L->HasCustomLayerDataKey(key)) {
      return true;
    }
  }
  return false;
}

std::vector<std::string> Stage::ListComposedCustomLayerDataKeys() const {
  std::set<std::string> keys;
  for (const std::shared_ptr<freeusd::sdf::Layer>& L : compose_) {
    if (!L) {
      continue;
    }
    for (const std::string& k : L->ListCustomLayerDataKeys()) {
      keys.insert(k);
    }
  }
  return std::vector<std::string>(keys.begin(), keys.end());
}

bool Stage::GetComposedPrimVariantSelection(const freeusd::sdf::Path& prim_path, const std::string& variantSet,
                                            std::string* outName) const {
  if (!outName || variantSet.empty()) {
    return false;
  }
  const freeusd::sdf::Path authored = map_composed_to_authored_path(compose_, prim_path);
  if (map_authored_to_composed_path(compose_, authored) != prim_path) {
    return false;
  }
  for (const std::shared_ptr<freeusd::sdf::Layer>& L : compose_) {
    if (L && L->HasPrimVariantSelectionKey(authored, variantSet)) {
      return L->GetPrimVariantSelectionEntry(authored, variantSet, outName);
    }
  }
  return false;
}

bool Stage::PrimVariantSelectionSetInAnyLayer(const freeusd::sdf::Path& prim_path,
                                              const std::string& variantSet) const {
  if (variantSet.empty()) {
    return false;
  }
  const freeusd::sdf::Path authored = map_composed_to_authored_path(compose_, prim_path);
  if (map_authored_to_composed_path(compose_, authored) != prim_path) {
    return false;
  }
  for (const std::shared_ptr<freeusd::sdf::Layer>& L : compose_) {
    if (L && L->HasPrimVariantSelectionKey(authored, variantSet)) {
      return true;
    }
  }
  return false;
}

std::vector<std::string> Stage::ListComposedPrimVariantSelectionSets(const freeusd::sdf::Path& prim_path) const {
  std::set<std::string> keys;
  const freeusd::sdf::Path authored = map_composed_to_authored_path(compose_, prim_path);
  if (map_authored_to_composed_path(compose_, authored) != prim_path) {
    return {};
  }
  for (const std::shared_ptr<freeusd::sdf::Layer>& L : compose_) {
    if (!L) {
      continue;
    }
    for (const std::string& k : L->ListPrimVariantSelectionSets(authored)) {
      keys.insert(k);
    }
  }
  return std::vector<std::string>(keys.begin(), keys.end());
}

std::vector<std::string> Stage::ListComposedPrimVariantSetNames(const freeusd::sdf::Path& prim_path) const {
  std::set<std::string> names;
  const freeusd::sdf::Path authored = map_composed_to_authored_path(compose_, prim_path);
  if (map_authored_to_composed_path(compose_, authored) != prim_path) {
    return {};
  }
  for (const std::shared_ptr<freeusd::sdf::Layer>& L : compose_) {
    if (!L) {
      continue;
    }
    for (const std::string& n : L->ListPrimVariantSetNames(authored)) {
      names.insert(n);
    }
  }
  return std::vector<std::string>(names.begin(), names.end());
}

bool Stage::PrimVariantSetDeclaredInAnyLayer(const freeusd::sdf::Path& prim_path,
                                             const std::string& variantSetName) const {
  if (variantSetName.empty()) {
    return false;
  }
  const freeusd::sdf::Path authored = map_composed_to_authored_path(compose_, prim_path);
  if (map_authored_to_composed_path(compose_, authored) != prim_path) {
    return false;
  }
  for (const std::shared_ptr<freeusd::sdf::Layer>& L : compose_) {
    if (L && L->HasPrimVariantSet(authored, variantSetName)) {
      return true;
    }
  }
  return false;
}

std::vector<std::string> Stage::GetComposedPrimVariantNames(const freeusd::sdf::Path& prim_path,
                                                            const std::string& variantSetName) const {
  if (variantSetName.empty()) {
    return {};
  }
  const freeusd::sdf::Path authored = map_composed_to_authored_path(compose_, prim_path);
  if (map_authored_to_composed_path(compose_, authored) != prim_path) {
    return {};
  }
  for (const std::shared_ptr<freeusd::sdf::Layer>& L : compose_) {
    if (L && L->HasPrimVariantSet(authored, variantSetName)) {
      return L->ListPrimVariantNames(authored, variantSetName);
    }
  }
  return {};
}

std::vector<std::string> Stage::ListComposedFieldNames(const freeusd::sdf::Path& prim_path) const {
  std::set<std::string> keys;
  const freeusd::sdf::Path authored = map_composed_to_authored_path(compose_, prim_path);
  if (map_authored_to_composed_path(compose_, authored) != prim_path) {
    return {};
  }
  for (const std::shared_ptr<freeusd::sdf::Layer>& L : compose_) {
    if (!L) {
      continue;
    }
    for (const std::string& k : L->ListFieldNames(authored)) {
      keys.insert(k);
    }
  }
  static thread_local std::unordered_set<std::string> active_queries;
  const std::string active_key =
      std::to_string(reinterpret_cast<std::uintptr_t>(this)) + "|fields|" + authored.GetString();
  if (!active_queries.insert(active_key).second) {
    return std::vector<std::string>(keys.begin(), keys.end());
  }
  ActiveQueryGuard active_guard{&active_queries, active_key};
  const freeusd::sdf::Path variant_root = freeusd::sdf::Path::FromString("/__VariantRoot");
  for (const std::shared_ptr<freeusd::sdf::Layer>& layer : compose_) {
    if (!layer) {
      continue;
    }
    for (const freeusd::sdf::Path& ancestor : prim_ancestors_deepest_first(authored)) {
      freeusd::sdf::Path mapped_query;
      for (const std::string& set_name : layer->ListPrimVariantSetNames(ancestor)) {
        std::string selected;
        std::string payload_body;
        if (!GetComposedPrimVariantSelection(map_authored_to_composed_path(compose_, ancestor), set_name, &selected) ||
            !layer->GetPrimVariantPayload(ancestor, set_name, selected, &payload_body) ||
            !map_subtree_query_path(authored, ancestor, variant_root, &mapped_query)) {
          continue;
        }
        std::shared_ptr<Stage> variant_stage = build_variant_stage_from_payload(payload_body);
        if (!variant_stage) {
          continue;
        }
        for (const std::string& k : variant_stage->ListComposedFieldNames(mapped_query)) {
          keys.insert(k);
        }
      }
      const auto pull_reference_fields = [&](const std::vector<freeusd::sdf::PrimReference>& refs) {
        for (const freeusd::sdf::PrimReference& ref : refs) {
          std::shared_ptr<Stage> target_stage = open_asset_stage_from_reference(*layer, compose_, ref);
          if (!target_stage || !map_subtree_query_path(authored, ancestor, reference_target_root(*target_stage, ref), &mapped_query)) {
            continue;
          }
          for (const std::string& k : target_stage->ListComposedFieldNames(mapped_query)) {
            keys.insert(k);
          }
        }
      };
      pull_reference_fields(layer->ListPrimReferences(ancestor));
      pull_reference_fields(layer->ListPrimPayloads(ancestor));
      const auto pull_internal_fields = [&](const std::vector<freeusd::sdf::Path>& targets) {
        for (const freeusd::sdf::Path& target_root : targets) {
          if (!map_subtree_query_path(authored, ancestor, target_root, &mapped_query)) {
            continue;
          }
          for (const std::string& k : ListComposedFieldNames(map_authored_to_composed_path(compose_, mapped_query))) {
            keys.insert(k);
          }
        }
      };
      pull_internal_fields(layer->ListPrimInherits(ancestor));
      pull_internal_fields(layer->ListPrimSpecializes(ancestor));
    }
  }
  return std::vector<std::string>(keys.begin(), keys.end());
}

std::vector<double> Stage::ListComposedFieldSampleTimes(const freeusd::sdf::Path& prim_path,
                                                         const freeusd::tf::Token& name) const {
  std::set<double> times;
  const freeusd::sdf::Path authored = map_composed_to_authored_path(compose_, prim_path);
  if (map_authored_to_composed_path(compose_, authored) != prim_path) {
    return {};
  }
  if (!name.IsEmpty()) {
    for (std::size_t i = 0; i < compose_.size(); ++i) {
      const std::shared_ptr<freeusd::sdf::Layer>& L = compose_[i];
      if (!L) {
        continue;
      }
      const freeusd::sdf::LayerOffset& offset = layer_offset_for_index(compose_offsets_, i);
      for (const double t : L->ListSampleTimes(authored, name)) {
        times.insert(map_layer_time_to_composed_time(offset, t));
      }
    }
  }
  return std::vector<double>(times.begin(), times.end());
}

std::vector<std::string> Stage::ListComposedRelationshipNames(const freeusd::sdf::Path& prim_path) const {
  std::set<std::string> keys;
  const freeusd::sdf::Path authored = map_composed_to_authored_path(compose_, prim_path);
  if (map_authored_to_composed_path(compose_, authored) != prim_path) {
    return {};
  }
  for (const std::shared_ptr<freeusd::sdf::Layer>& L : compose_) {
    if (!L) {
      continue;
    }
    for (const std::string& k : L->ListRelationshipNames(authored)) {
      keys.insert(k);
    }
  }
  return std::vector<std::string>(keys.begin(), keys.end());
}

std::vector<freeusd::sdf::Path> Stage::ListComposedPrimPaths() const {
  std::vector<freeusd::sdf::Path> out;
  std::unordered_set<std::string> seen;
  for (const std::shared_ptr<freeusd::sdf::Layer>& layer : compose_) {
    if (!layer) {
      continue;
    }
    for (const freeusd::sdf::Path& p : layer->ListPrimPaths()) {
      const freeusd::sdf::Path composed = map_authored_to_composed_path(compose_, p);
      if ((composed.IsPrimPath() || composed.IsAbsoluteRootPath()) && seen.insert(composed.GetString()).second) {
        out.push_back(composed);
      }
    }
  }
  const std::function<void(const freeusd::sdf::Path&)> descend = [&](const freeusd::sdf::Path& parent) {
    for (const Prim& child : GetChildren(parent)) {
      const std::string key = child.GetPath().GetString();
      if (!seen.insert(key).second) {
        continue;
      }
      out.push_back(child.GetPath());
      descend(child.GetPath());
    }
  };
  descend(freeusd::sdf::Path::AbsoluteRootPath());
  std::sort(out.begin(), out.end(), [](const freeusd::sdf::Path& a, const freeusd::sdf::Path& b) {
    return a.GetString() < b.GetString();
  });
  return out;
}

bool Stage::GetComposedRelocateTarget(const freeusd::sdf::Path& fromPrimPath, freeusd::sdf::Path* outToPrimPath) const {
  if (!outToPrimPath || !fromPrimPath.IsPrimPath()) {
    return false;
  }
  for (const std::shared_ptr<freeusd::sdf::Layer>& L : compose_) {
    if (L && L->HasRelocate(fromPrimPath)) {
      return L->GetRelocateTarget(fromPrimPath, outToPrimPath);
    }
  }
  return false;
}

bool Stage::RelocateSourceInAnyLayer(const freeusd::sdf::Path& fromPrimPath) const {
  if (!fromPrimPath.IsPrimPath()) {
    return false;
  }
  for (const std::shared_ptr<freeusd::sdf::Layer>& L : compose_) {
    if (L && L->HasRelocate(fromPrimPath)) {
      return true;
    }
  }
  return false;
}

std::vector<std::pair<freeusd::sdf::Path, freeusd::sdf::Path>> Stage::ListComposedRelocates() const {
  return list_composed_relocates(compose_);
}

bool Stage::GetComposedPrefixSubstitution(const std::string& fromPrefix, std::string* outToPrefix) const {
  if (!outToPrefix || fromPrefix.empty()) {
    return false;
  }
  for (const std::shared_ptr<freeusd::sdf::Layer>& L : compose_) {
    if (L && L->HasPrefixSubstitution(fromPrefix)) {
      return L->GetPrefixSubstitution(fromPrefix, outToPrefix);
    }
  }
  return false;
}

bool Stage::PrefixSubstitutionKeyInAnyLayer(const std::string& fromPrefix) const {
  if (fromPrefix.empty()) {
    return false;
  }
  for (const std::shared_ptr<freeusd::sdf::Layer>& L : compose_) {
    if (L && L->HasPrefixSubstitution(fromPrefix)) {
      return true;
    }
  }
  return false;
}

std::vector<std::pair<std::string, std::string>> Stage::ListComposedPrefixSubstitutions() const {
  return list_composed_prefix_substitutions(compose_);
}

bool Stage::HasDefaultPrim() const {
  const std::shared_ptr<freeusd::sdf::Layer> r = GetRootLayerPtr();
  return static_cast<bool>(r && r->HasDefaultPrim());
}

std::string Stage::GetDefaultPrimName() const {
  const std::shared_ptr<freeusd::sdf::Layer> r = GetRootLayerPtr();
  if (!r || !r->HasDefaultPrim()) {
    return {};
  }
  const std::optional<std::string_view> dp = r->GetDefaultPrim();
  if (!dp.has_value()) {
    return {};
  }
  return std::string{*dp};
}

Prim Stage::GetDefaultPrim() const {
  if (!HasDefaultPrim()) {
    return {};
  }
  const std::string n = GetDefaultPrimName();
  if (n.empty()) {
    return {};
  }
  const freeusd::sdf::Path p =
      freeusd::sdf::Path::AbsoluteRootPath().AppendChild(freeusd::tf::Token(n));
  return GetPrimAtPath(map_authored_to_composed_path(compose_, p));
}

std::optional<double> Stage::GetStartTimeCode() const {
  for (const std::shared_ptr<freeusd::sdf::Layer>& L : compose_) {
    if (!L) {
      continue;
    }
    const std::optional<double> v = L->GetStartTimeCode();
    if (v.has_value()) {
      return v;
    }
  }
  return std::nullopt;
}

std::optional<double> Stage::GetEndTimeCode() const {
  for (const std::shared_ptr<freeusd::sdf::Layer>& L : compose_) {
    if (!L) {
      continue;
    }
    const std::optional<double> v = L->GetEndTimeCode();
    if (v.has_value()) {
      return v;
    }
  }
  return std::nullopt;
}

std::optional<double> Stage::GetTimeCodesPerSecond() const {
  for (const std::shared_ptr<freeusd::sdf::Layer>& L : compose_) {
    if (!L) {
      continue;
    }
    const std::optional<double> v = L->GetTimeCodesPerSecond();
    if (v.has_value()) {
      return v;
    }
  }
  return std::nullopt;
}

std::optional<double> Stage::GetFramesPerSecond() const {
  for (const std::shared_ptr<freeusd::sdf::Layer>& L : compose_) {
    if (!L) {
      continue;
    }
    const std::optional<double> v = L->GetFramesPerSecond();
    if (v.has_value()) {
      return v;
    }
  }
  return std::nullopt;
}

std::optional<int> Stage::GetFramePrecision() const {
  for (const std::shared_ptr<freeusd::sdf::Layer>& L : compose_) {
    if (!L) {
      continue;
    }
    const std::optional<int> v = L->GetFramePrecision();
    if (v.has_value()) {
      return v;
    }
  }
  return std::nullopt;
}

std::optional<double> Stage::GetMetersPerUnit() const {
  for (const std::shared_ptr<freeusd::sdf::Layer>& L : compose_) {
    if (!L) {
      continue;
    }
    const std::optional<double> v = L->GetMetersPerUnit();
    if (v.has_value()) {
      return v;
    }
  }
  return std::nullopt;
}

std::optional<std::string> Stage::GetUpAxis() const {
  for (const std::shared_ptr<freeusd::sdf::Layer>& L : compose_) {
    if (!L) {
      continue;
    }
    const std::optional<std::string> v = L->GetUpAxis();
    if (v.has_value()) {
      return v;
    }
  }
  return std::nullopt;
}

std::vector<freeusd::sdf::Path> Stage::GetPrimOrder() const {
  for (const std::shared_ptr<freeusd::sdf::Layer>& L : compose_) {
    if (!L) {
      continue;
    }
    const std::vector<freeusd::sdf::Path>& po = L->GetPrimOrder();
    if (!po.empty()) {
      return po;
    }
  }
  return {};
}

freeusd::sdf::Path Stage::GetPseudoRootPath() const { return freeusd::sdf::Path::AbsoluteRootPath(); }

Prim Stage::GetPrimAtPath(const freeusd::sdf::Path& path) const {
  if (!path.IsPrimPath()) {
    return {};
  }
  return Prim{weak_from_this(), path};
}

void Stage::TraversePreorder(const std::function<bool(const Prim& prim)>& visitor) const {
  if (!visitor) {
    return;
  }
  const std::weak_ptr<const Stage> wp = weak_from_this();
  const std::function<void(const freeusd::sdf::Path&)> descend = [&](const freeusd::sdf::Path& path) {
    const Prim prim{wp, path};
    if (!prim.IsValid()) {
      return;
    }
    if (!visitor(prim)) {
      return;
    }
    for (const Prim& ch : GetChildren(path)) {
      descend(ch.GetPath());
    }
  };
  for (const Prim& root : GetChildren(freeusd::sdf::Path::AbsoluteRootPath())) {
    descend(root.GetPath());
  }
}

std::vector<Prim> Stage::GetChildren(const freeusd::sdf::Path& primPath) const {
  std::vector<Prim> out;
  if (!primPath.IsPrimPath() && !primPath.IsAbsoluteRootPath()) {
    return out;
  }
  std::unordered_map<std::string, freeusd::sdf::Path> unique;
  const auto add_child = [&](const freeusd::sdf::Path& child_path) {
    if (child_path.IsPrimPath() && child_path.GetParentPath() == primPath) {
      unique[child_path.GetString()] = child_path;
    }
  };
  for (const std::shared_ptr<freeusd::sdf::Layer>& layer : compose_) {
    if (!layer) {
      continue;
    }
    for (const freeusd::sdf::Path& p : layer->ListPrimPaths()) {
      add_child(map_authored_to_composed_path(compose_, p));
    }
  }
  static thread_local std::unordered_set<std::string> active_queries;
  const std::string active_key =
      std::to_string(reinterpret_cast<std::uintptr_t>(this)) + "|children|" + primPath.GetString();
  if (active_queries.insert(active_key).second) {
    ActiveQueryGuard active_guard{&active_queries, active_key};
    const freeusd::sdf::Path authored_parent =
        primPath.IsAbsoluteRootPath() ? primPath : map_composed_to_authored_path(compose_, primPath);
    const freeusd::sdf::Path variant_root = freeusd::sdf::Path::FromString("/__VariantRoot");
    for (const std::shared_ptr<freeusd::sdf::Layer>& layer : compose_) {
      if (!layer) {
        continue;
      }
      const std::vector<freeusd::sdf::Path> ancestors =
          authored_parent.IsPrimPath() ? prim_ancestors_deepest_first(authored_parent) : std::vector<freeusd::sdf::Path>{};
      for (const freeusd::sdf::Path& ancestor : ancestors) {
        freeusd::sdf::Path mapped_parent;
        for (const std::string& set_name : layer->ListPrimVariantSetNames(ancestor)) {
          std::string selected;
          std::string payload_body;
          if (!GetComposedPrimVariantSelection(map_authored_to_composed_path(compose_, ancestor), set_name, &selected) ||
              !layer->GetPrimVariantPayload(ancestor, set_name, selected, &payload_body) ||
              !map_subtree_query_path(authored_parent, ancestor, variant_root, &mapped_parent)) {
            continue;
          }
          std::shared_ptr<Stage> variant_stage = build_variant_stage_from_payload(payload_body);
          if (!variant_stage) {
            continue;
          }
          for (const Prim& child : variant_stage->GetChildren(mapped_parent)) {
            freeusd::sdf::Path mapped_back;
            if (map_subtree_query_path(child.GetPath(), mapped_parent, ancestor, &mapped_back)) {
              add_child(map_authored_to_composed_path(compose_, mapped_back));
            }
          }
        }
        const auto pull_reference_children = [&](const std::vector<freeusd::sdf::PrimReference>& refs) {
          for (const freeusd::sdf::PrimReference& ref : refs) {
            std::shared_ptr<Stage> target_stage = open_asset_stage_from_reference(*layer, compose_, ref);
            if (!target_stage || !map_subtree_query_path(authored_parent, ancestor, reference_target_root(*target_stage, ref), &mapped_parent)) {
              continue;
            }
            for (const Prim& child : target_stage->GetChildren(mapped_parent)) {
              freeusd::sdf::Path mapped_back;
              if (map_subtree_query_path(child.GetPath(), mapped_parent, ancestor, &mapped_back)) {
                add_child(map_authored_to_composed_path(compose_, mapped_back));
              }
            }
          }
        };
        pull_reference_children(layer->ListPrimReferences(ancestor));
        pull_reference_children(layer->ListPrimPayloads(ancestor));
        const auto pull_internal_children = [&](const std::vector<freeusd::sdf::Path>& targets) {
          for (const freeusd::sdf::Path& target_root : targets) {
            if (!map_subtree_query_path(authored_parent, ancestor, target_root, &mapped_parent)) {
              continue;
            }
            for (const Prim& child : GetChildren(map_authored_to_composed_path(compose_, mapped_parent))) {
              freeusd::sdf::Path mapped_back;
              if (map_subtree_query_path(map_composed_to_authored_path(compose_, child.GetPath()), mapped_parent, ancestor, &mapped_back)) {
                add_child(map_authored_to_composed_path(compose_, mapped_back));
              }
            }
          }
        };
        pull_internal_children(layer->ListPrimInherits(ancestor));
        pull_internal_children(layer->ListPrimSpecializes(ancestor));
      }
    }
  }
  out.reserve(unique.size());
  for (const auto& e : unique) {
    out.emplace_back(weak_from_this(), e.second);
  }
  std::sort(out.begin(), out.end(), [](const Prim& a, const Prim& b) {
    return a.GetPath().GetString() < b.GetPath().GetString();
  });
  return out;
}

void Stage::SetResolver(std::unique_ptr<freeusd::ar::Resolver> resolver) {
  if (resolver) {
    resolver_ = std::move(resolver);
  }
}

}  // namespace freeusd::usd
