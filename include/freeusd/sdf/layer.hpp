#pragma once

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "freeusd/export.hpp"
#include "freeusd/sdf/fieldOpinion.hpp"
#include "freeusd/sdf/path.hpp"
#include "freeusd/tf/hash.hpp"
#include "freeusd/tf/token.hpp"
#include "freeusd/vt/value.hpp"

namespace freeusd::sdf {

/// In-memory layer with prim fields (SdfLayer-shaped surface, clean-room).
class FREEUSD_API Layer : public std::enable_shared_from_this<Layer> {
 public:
  static std::shared_ptr<Layer> NewAnonymous(std::string identifier = {});

  const std::string& GetIdentifier() const noexcept { return identifier_; }
  void SetIdentifier(std::string id) { identifier_ = std::move(id); }

  /// Author a default value (does not remove existing time samples).
  void SetField(const Path& primPath, const freeusd::tf::Token& name, const freeusd::vt::Value& value);

  /// True if there is a default and/or any time sample for `name` (base attribute name, no `.timeSamples` suffix).
  bool HasField(const Path& primPath, const freeusd::tf::Token& name) const;

  /// Reads the authored default only (not evaluated samples). Fails if no default was set.
  bool GetField(const Path& primPath, const freeusd::tf::Token& name, freeusd::vt::Value* out) const;

  /// Evaluated value at `time` using **hold** (last sample with time <= `time`, else earliest sample; if no samples, default).
  bool GetFieldAtTime(const Path& primPath, const freeusd::tf::Token& name, double time, freeusd::vt::Value* out) const;

  void SetTimeSample(const Path& primPath, const freeusd::tf::Token& name, double time, const freeusd::vt::Value& value);
  std::vector<double> ListSampleTimes(const Path& primPath, const freeusd::tf::Token& name) const;
  bool GetTimeSample(const Path& primPath, const freeusd::tf::Token& name, double time, freeusd::vt::Value* out) const;
  void ClearTimeSamples(const Path& primPath, const freeusd::tf::Token& name);

  /// Field names authored on a prim (empty if none).
  std::vector<std::string> ListFieldNames(const Path& primPath) const;

  /// All prim paths that have any authored fields.
  std::vector<Path> ListPrimPaths() const;

  /// Remove all opinions (paths, fields, kinds).
  void Clear() noexcept;

  /// Schema kind from `def Type "Name"` (e.g. Xform, Mesh). Empty token clears.
  void SetPrimKind(const Path& primPath, const freeusd::tf::Token& kind);
  bool HasPrimKind(const Path& primPath) const;
  freeusd::tf::Token GetPrimKind(const Path& primPath) const;

  /// Layer header: human-readable summary (USD `documentation` mapping).
  const std::string& GetDocumentation() const noexcept { return documentation_; }
  void SetDocumentation(std::string doc) { documentation_ = std::move(doc); }

  void SetDefaultPrim(std::string_view rootPrimName);
  void ClearDefaultPrim() noexcept { default_prim_.reset(); }
  bool HasDefaultPrim() const noexcept { return default_prim_.has_value(); }
  std::optional<std::string_view> GetDefaultPrim() const noexcept {
    if (!default_prim_) {
      return std::nullopt;
    }
    return std::string_view{*default_prim_};
  }

  void SetSubLayers(std::vector<std::string> paths);
  void ClearSubLayers() noexcept { sublayer_paths_.clear(); }
  const std::vector<std::string>& GetSubLayers() const noexcept { return sublayer_paths_; }

  /// Reference arc asset paths authored on prim (minimal Pcp precursor; ordering preserved).
  void AddReference(const Path& primPath, std::string assetPath);
  void SetReferences(const Path& primPath, std::vector<std::string> paths);
  std::vector<std::string> ListReferences(const Path& primPath) const;
  void ClearReferences(const Path& primPath);

  bool IsPrimActive(const Path& primPath) const noexcept;
  void SetPrimActive(const Path& primPath, bool active);
  bool HasPrimActiveOpinion(const Path& primPath) const noexcept;

  /// Explicit `specifier` marker for USDA round-trip (`def`, `class`, `over`; default behaves as `def`).
  enum class PrimSpecifierKind { Default, Def, Class, Over };

  void SetPrimSpecifier(const Path& primPath, PrimSpecifierKind k);
  PrimSpecifierKind GetPrimSpecifier(const Path& primPath) const noexcept;

 private:
  explicit Layer(std::string identifier);

  void touch_hierarchy(const Path& primPath);

  FieldOpinion* find_opinion(const Path& primPath, const std::string& name);
  const FieldOpinion* find_opinion(const Path& primPath, const std::string& name) const;

  using FieldMap = std::unordered_map<std::string, FieldOpinion, freeusd::tf::HashString, std::equal_to<>>;

  std::string identifier_;
  std::string documentation_;
  std::optional<std::string> default_prim_;
  std::vector<std::string> sublayer_paths_;

  std::unordered_map<Path, FieldMap, Path::Hash> fields_;
  std::unordered_set<Path, Path::Hash> hierarchy_;
  std::unordered_map<Path, freeusd::tf::Token, Path::Hash> prim_kinds_;
  std::unordered_map<Path, std::vector<std::string>, Path::Hash> references_;
  std::unordered_map<Path, bool, Path::Hash> prim_active_;
  std::unordered_map<Path, PrimSpecifierKind, Path::Hash> prim_specifiers_;
};

}  // namespace freeusd::sdf
