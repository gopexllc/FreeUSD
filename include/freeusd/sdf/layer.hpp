#pragma once

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "freeusd/export.hpp"
#include "freeusd/sdf/fieldOpinion.hpp"
#include "freeusd/sdf/layerOffset.hpp"
#include "freeusd/sdf/path.hpp"
#include "freeusd/sdf/primReference.hpp"
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

  /// Prim-scope `customData` dictionary (Usd-style metadata; string keys → \c Value payloads).
  void ClearPrimCustomData(const Path& primPath);
  void SetPrimCustomDataEntry(const Path& primPath, std::string key, const freeusd::vt::Value& value);
  void ErasePrimCustomDataEntry(const Path& primPath, const std::string& key);
  bool HasPrimCustomDataKey(const Path& primPath, const std::string& key) const;
  bool GetPrimCustomDataEntry(const Path& primPath, const std::string& key, freeusd::vt::Value* out) const;
  std::vector<std::string> ListPrimCustomDataKeys(const Path& primPath) const;

  /// Prim-scope `variantSelection` map (variant set name → selected variant name). Does not expand variants in composition.
  void ClearPrimVariantSelection(const Path& primPath);
  void SetPrimVariantSelectionEntry(const Path& primPath, std::string variantSet, std::string variantName);
  void ErasePrimVariantSelectionEntry(const Path& primPath, const std::string& variantSet);
  bool HasPrimVariantSelectionKey(const Path& primPath, const std::string& variantSet) const;
  bool GetPrimVariantSelectionEntry(const Path& primPath, const std::string& variantSet, std::string* outName) const;
  std::vector<std::string> ListPrimVariantSelectionSets(const Path& primPath) const;

  /// Declared \c variantSets: variant set name → ordered variant child names (no variant payload prims).
  void ClearPrimVariantSets(const Path& primPath);
  void SetPrimVariantSetVariants(const Path& primPath, std::string variantSetName, std::vector<std::string> variantNames);
  bool HasPrimVariantSet(const Path& primPath, const std::string& variantSetName) const;
  std::vector<std::string> ListPrimVariantSetNames(const Path& primPath) const;
  std::vector<std::string> ListPrimVariantNames(const Path& primPath, const std::string& variantSetName) const;

  /// Layer header: human-readable summary (USD `documentation` mapping).
  const std::string& GetDocumentation() const noexcept { return documentation_; }
  void SetDocumentation(std::string doc) { documentation_ = std::move(doc); }

  /// Layer header \c comment (SdfLayer \c comment; distinct from \c documentation).
  const std::string& GetComment() const noexcept { return comment_; }
  void SetComment(std::string text) { comment_ = std::move(text); }
  void ClearComment() noexcept { comment_.clear(); }

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

  /// Per-sublayer time mapping (USD \c subLayerOffsets; keys are authored asset path strings, e.g. \c @./a.usda@).
  void ClearSubLayerOffsets() noexcept;
  void SetSubLayerOffset(std::string authoredSublayerPath, LayerOffset offset);
  void EraseSubLayerOffset(const std::string& authoredSublayerPath);
  bool HasSubLayerOffset(const std::string& authoredSublayerPath) const;
  bool GetSubLayerOffset(const std::string& authoredSublayerPath, LayerOffset* out) const;
  /// Pairs sorted by sublayer path for stable USDA output.
  std::vector<std::pair<std::string, LayerOffset>> ListSubLayerOffsets() const;

  /// Layer-scope \c relocates map (source absolute prim path → target absolute prim path). Stored only; no graph rewrite.
  void ClearRelocates() noexcept;
  void SetRelocate(Path fromPrimPath, Path toPrimPath);
  void EraseRelocate(const Path& fromPrimPath);
  bool HasRelocate(const Path& fromPrimPath) const;
  bool GetRelocateTarget(const Path& fromPrimPath, Path* outToPrimPath) const;
  /// Sorted by source path string for stable USDA output.
  std::vector<std::pair<Path, Path>> ListRelocates() const;

  /// Layer-scope \c prefixSubstitutions (prefix string → replacement string). Stored only; no path rewriting in queries.
  void ClearPrefixSubstitutions() noexcept;
  void SetPrefixSubstitution(std::string fromPrefix, std::string toPrefix);
  void ErasePrefixSubstitution(const std::string& fromPrefix);
  bool HasPrefixSubstitution(const std::string& fromPrefix) const;
  bool GetPrefixSubstitution(const std::string& fromPrefix, std::string* outToPrefix) const;
  /// Pairs sorted by \p fromPrefix for stable USDA output.
  std::vector<std::pair<std::string, std::string>> ListPrefixSubstitutions() const;

  /// Layer-scope \c customLayerData (string keys → \c Value). Same role as SdfLayer \c customLayerData.
  void ClearCustomLayerData() noexcept;
  void SetCustomLayerDataEntry(std::string key, const freeusd::vt::Value& value);
  void EraseCustomLayerDataEntry(const std::string& key);
  bool HasCustomLayerDataKey(const std::string& key) const;
  bool GetCustomLayerDataEntry(const std::string& key, freeusd::vt::Value* out) const;
  std::vector<std::string> ListCustomLayerDataKeys() const;

  /// Reference arcs on prim (@asset@ with optional </Prim> tail), ordering preserved—minimal Pcp precursor.
  void AddPrimReference(const Path& primPath, PrimReference ref);
  void AddReference(const Path& primPath, std::string authoredFragment);
  void SetPrimReferences(const Path& primPath, std::vector<PrimReference> refs);
  void SetReferences(const Path& primPath, std::vector<std::string> authoredFragments);
  std::vector<PrimReference> ListPrimReferences(const Path& primPath) const;
  /// Canonical authored strings (@@path@@ and optional prim tail) for debugging and interoperability.
  std::vector<std::string> ListReferences(const Path& primPath) const;
  void ClearReferences(const Path& primPath);

  /// \c inherits arcs: absolute prim paths (USD \c inherits / \c prepend inherits / \c append inherits).
  void ClearPrimInherits(const Path& primPath);
  void AddPrimInherit(const Path& primPath, Path targetPrimPath);
  void PrependPrimInherits(const Path& primPath, std::vector<Path> front);
  void AppendPrimInherits(const Path& primPath, std::vector<Path> back);
  void SetPrimInherits(const Path& primPath, std::vector<Path> targets);
  std::vector<Path> ListPrimInherits(const Path& primPath) const;

  /// \c specializes arcs (absolute prim paths).
  void ClearPrimSpecializes(const Path& primPath);
  void AddPrimSpecializes(const Path& primPath, Path targetPrimPath);
  void PrependPrimSpecializes(const Path& primPath, std::vector<Path> front);
  void AppendPrimSpecializes(const Path& primPath, std::vector<Path> back);
  void SetPrimSpecializes(const Path& primPath, std::vector<Path> targets);
  std::vector<Path> ListPrimSpecializes(const Path& primPath) const;

  /// \c payload arcs (\c PrimReference list; same authored shape as references).
  void ClearPrimPayloads(const Path& primPath);
  void AddPrimPayload(const Path& primPath, PrimReference ref);
  void SetPrimPayloads(const Path& primPath, std::vector<PrimReference> refs);
  std::vector<PrimReference> ListPrimPayloads(const Path& primPath) const;
  std::vector<std::string> ListPayloads(const Path& primPath) const;

  /// Relationship authored targets (UsdRelationship / SdfPathVector-shaped; strongest-first list order preserved).
  void SetRelationshipTargets(const Path& primPath, const freeusd::tf::Token& relName, std::vector<Path> targets);
  void PrependRelationshipTargets(const Path& primPath, const freeusd::tf::Token& relName, std::vector<Path> extraFront);
  void AppendRelationshipTargets(const Path& primPath, const freeusd::tf::Token& relName, std::vector<Path> extraBack);
  /// Remove every occurrence of each path in \p toRemove from the authored target list (no-op if the rel is missing).
  void DeleteRelationshipTargets(const Path& primPath, const freeusd::tf::Token& relName, std::vector<Path> toRemove);
  void ClearRelationship(const Path& primPath, const freeusd::tf::Token& relName);
  bool HasRelationship(const Path& primPath, const freeusd::tf::Token& relName) const;
  std::vector<Path> GetRelationshipTargets(const Path& primPath, const freeusd::tf::Token& relName) const;
  std::vector<std::string> ListRelationshipNames(const Path& primPath) const;

  /// Attribute connection (`name.connect`): \p targetProp must be a full property path (e.g. \c /Mesh.radius).
  void SetAttributeConnection(const Path& primPath, const freeusd::tf::Token& name, Path targetProp);
  void ClearAttributeConnection(const Path& primPath, const freeusd::tf::Token& name);
  bool HasAttributeConnection(const Path& primPath, const freeusd::tf::Token& name) const;
  bool GetAttributeConnectionTarget(const Path& primPath, const freeusd::tf::Token& name, Path* targetProp) const;
  std::vector<std::pair<std::string, Path>> ListAttributeConnections(const Path& primPath) const;

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
  using RelMap = std::unordered_map<std::string, std::vector<Path>, freeusd::tf::HashString, std::equal_to<>>;

  std::string identifier_;
  std::string documentation_;
  std::string comment_;
  std::optional<std::string> default_prim_;
  std::vector<std::string> sublayer_paths_;
  std::unordered_map<std::string, LayerOffset, freeusd::tf::HashString, std::equal_to<>> sublayer_offsets_;
  std::unordered_map<Path, Path, Path::Hash> relocates_;
  std::unordered_map<std::string, std::string, freeusd::tf::HashString, std::equal_to<>> prefix_substitutions_;
  std::unordered_map<std::string, freeusd::vt::Value, freeusd::tf::HashString, std::equal_to<>> custom_layer_data_;

  std::unordered_map<Path, FieldMap, Path::Hash> fields_;
  std::unordered_set<Path, Path::Hash> hierarchy_;
  std::unordered_map<Path, freeusd::tf::Token, Path::Hash> prim_kinds_;
  std::unordered_map<Path, std::vector<PrimReference>, Path::Hash> references_;
  std::unordered_map<Path, std::vector<Path>, Path::Hash> prim_inherits_;
  std::unordered_map<Path, std::vector<Path>, Path::Hash> prim_specializes_;
  std::unordered_map<Path, std::vector<PrimReference>, Path::Hash> prim_payloads_;
  std::unordered_map<Path, RelMap, Path::Hash> relationships_;
  std::unordered_map<Path, bool, Path::Hash> prim_active_;
  std::unordered_map<Path, PrimSpecifierKind, Path::Hash> prim_specifiers_;
  std::unordered_map<Path, std::unordered_map<std::string, freeusd::vt::Value, freeusd::tf::HashString, std::equal_to<>>, Path::Hash>
      prim_custom_data_;
  std::unordered_map<Path, std::unordered_map<std::string, std::string, freeusd::tf::HashString, std::equal_to<>>, Path::Hash>
      prim_variant_selection_;
  std::unordered_map<Path, std::unordered_map<std::string, std::vector<std::string>, freeusd::tf::HashString, std::equal_to<>>,
                     Path::Hash>
      prim_variant_sets_;
  std::unordered_map<Path, std::unordered_map<std::string, Path, freeusd::tf::HashString, std::equal_to<>>, Path::Hash>
      attribute_connections_;
};

}  // namespace freeusd::sdf
