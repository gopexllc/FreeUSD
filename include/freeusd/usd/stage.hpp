#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "freeusd/ar/defaultResolver.hpp"
#include "freeusd/export.hpp"
#include "freeusd/pcp/layerStack.hpp"
#include "freeusd/sdf/layer.hpp"
#include "freeusd/tf/token.hpp"
#include "freeusd/usd/prim.hpp"

namespace freeusd::vt {
class Value;
}

namespace freeusd::usd {

/// How @c subLayers authored on a root layer file are expanded when opening a stage from disk.
enum class RootLayerSublayersPolicy {
  /// Load only the root file; ignore authored @c subLayers for stacking.
  None,
  /// Root (strongest) then each immediate @c subLayer entry (non-recursive).
  Shallow,
  /// Depth-first expansion of nested @c subLayers (see @ref freeusd::pcp::ComposeSublayersDepthFirst).
  DepthFirst,
};

class FREEUSD_API Stage : public std::enable_shared_from_this<Stage> {
 public:
  static std::shared_ptr<Stage> AttachRootLayer(std::shared_ptr<freeusd::sdf::Layer> root);

  /// Strongest layer is \c compose_layers()[0]; remainder are weaker (e.g. sublayers).
  static std::shared_ptr<Stage> AttachLayerStack(freeusd::pcp::LayerStack stack);

  /**
   * Load a USDA root file from disk and build a stage, optionally stacking authored @c subLayers.
   * Relative sublayer paths resolve against the root file's parent directory (Usd-style anchor).
   * Missing or unparseable sublayers are skipped (same tolerance as @ref freeusd::pcp::ComposeSublayers).
   * On failure returns null and optionally fills @p err_detail.
   */
  static std::shared_ptr<Stage> OpenFromRootFile(const std::string& layer_path,
                                                 RootLayerSublayersPolicy sublayers = RootLayerSublayersPolicy::DepthFirst,
                                                 std::string* err_detail = nullptr);

  const freeusd::sdf::Layer& GetRootLayer() const { return *compose_.front(); }
  std::shared_ptr<freeusd::sdf::Layer> GetRootLayerPtr() const {
    return compose_.empty() ? nullptr : compose_.front();
  }

  const std::vector<std::shared_ptr<freeusd::sdf::Layer>>& GetComposeLayers() const noexcept { return compose_; }

  /// Evaluated attribute at \p time (default + time samples per layer); strongest opinion wins.
  /// Returns @c false when @p out is null, @p name is empty, no composed value exists, or
  /// connection-following fails (invalid target / cycle / hop limit).
  bool ReadFieldAtEvaluatedTime(const freeusd::sdf::Path& prim_path, const freeusd::tf::Token& name, double time,
                                freeusd::vt::Value* out) const;

  /// True when any composed layer authors @p name on @p prim_path, including \c .connect opinions.
  bool HasFieldOpinion(const freeusd::sdf::Path& prim_path, const freeusd::tf::Token& name) const;

  /// True if any composed layer authors a @c .connect entry for @p attr_name on @p prim_path.
  bool HasAttributeConnection(const freeusd::sdf::Path& prim_path, const freeusd::tf::Token& attr_name) const;

  /// Strongest composed @c .connect target for @p attr_name (full property path). False if none.
  bool GetComposedAttributeConnectionTarget(const freeusd::sdf::Path& prim_path, const freeusd::tf::Token& attr_name,
                                            freeusd::sdf::Path* out_target) const;

  /// True if any composed layer lists this prim path (authored presence).
  bool PrimPathInUse(const freeusd::sdf::Path& path) const;

  /// Concatenate relationship targets from each layer that carries \p rel_name (stronger layers first).
  bool ReadRelationship(const freeusd::sdf::Path& prim_path, const freeusd::tf::Token& rel_name,
                        std::vector<freeusd::sdf::Path>* out_targets) const;

  /// True if any composed layer authors \p rel_name on \p prim_path.
  bool HasRelationship(const freeusd::sdf::Path& prim_path, const freeusd::tf::Token& rel_name) const;

  /// Concatenate \c references / \c inherits / \c specializes / \c payloads from each composed layer (stronger first).
  std::vector<freeusd::sdf::PrimReference> ReadPrimReferences(const freeusd::sdf::Path& prim_path) const;
  bool HasPrimReferences(const freeusd::sdf::Path& prim_path) const;
  std::vector<freeusd::sdf::Path> ReadPrimInherits(const freeusd::sdf::Path& prim_path) const;
  bool HasPrimInherits(const freeusd::sdf::Path& prim_path) const;
  std::vector<freeusd::sdf::Path> ReadPrimSpecializes(const freeusd::sdf::Path& prim_path) const;
  bool HasPrimSpecializes(const freeusd::sdf::Path& prim_path) const;
  std::vector<freeusd::sdf::PrimReference> ReadPrimPayloads(const freeusd::sdf::Path& prim_path) const;
  bool HasPrimPayloads(const freeusd::sdf::Path& prim_path) const;

  /// Strongest local @c kind opinion, else via references, payloads, @c inherits, and @c specializes.
  freeusd::tf::Token ResolvePrimKind(const freeusd::sdf::Path& prim_path) const;
  bool ResolveHasPrimKind(const freeusd::sdf::Path& prim_path) const;

  /// Composed USDA \c class / \c over specifier: strongest local opinion wins; else via `inherits` / `specializes`.
  freeusd::sdf::Layer::PrimSpecifierKind ResolvePrimSpecifierKind(const freeusd::sdf::Path& prim_path) const;

  /// Strongest local @c active opinion, else via references, payloads, @c inherits, and @c specializes (default @c true).
  bool ResolvePrimActive(const freeusd::sdf::Path& prim_path) const;
  bool ResolveHasPrimActiveOpinion(const freeusd::sdf::Path& prim_path) const;

  /// Composed `customData` entry: strongest local layer opinion wins; else inherited via `inherits` / `specializes`.
  bool GetComposedPrimCustomData(const freeusd::sdf::Path& prim_path, const std::string& key,
                                 freeusd::vt::Value* out) const;
  /// True iff any composed layer carries \p key in `customData` for \p prim_path.
  bool PrimCustomDataKeyInAnyLayer(const freeusd::sdf::Path& prim_path, const std::string& key) const;
  /// Stable union of all `customData` keys authored on \p prim_path across the layer stack.
  std::vector<std::string> ListComposedPrimCustomDataKeys(const freeusd::sdf::Path& prim_path) const;

  /// Composed layer \c customLayerData entry: strongest layer with an opinion wins.
  bool GetComposedCustomLayerData(const std::string& key, freeusd::vt::Value* out) const;
  bool CustomLayerDataKeyInAnyLayer(const std::string& key) const;
  /// Stable union of all \c customLayerData keys across the composed layer stack.
  std::vector<std::string> ListComposedCustomLayerDataKeys() const;

  /// Composed `variantSelection` for \p variantSet: strongest layer with an opinion wins.
  bool GetComposedPrimVariantSelection(const freeusd::sdf::Path& prim_path, const std::string& variantSet,
                                       std::string* outName) const;
  /// True iff any composed layer authors \p variantSet in `variantSelection` for \p prim_path.
  bool PrimVariantSelectionSetInAnyLayer(const freeusd::sdf::Path& prim_path, const std::string& variantSet) const;
  /// Stable union of variant set names authored in `variantSelection` on \p prim_path across the layer stack.
  std::vector<std::string> ListComposedPrimVariantSelectionSets(const freeusd::sdf::Path& prim_path) const;

  /// Union of \c variantSets keys authored on \p prim_path across composed layers.
  std::vector<std::string> ListComposedPrimVariantSetNames(const freeusd::sdf::Path& prim_path) const;
  /// True iff any composed layer declares \p variantSetName in \c variantSets for \p prim_path.
  bool PrimVariantSetDeclaredInAnyLayer(const freeusd::sdf::Path& prim_path, const std::string& variantSetName) const;
  /// Variant names for \p variantSetName from the strongest layer that declares that set (empty if none).
  std::vector<std::string> GetComposedPrimVariantNames(const freeusd::sdf::Path& prim_path,
                                                      const std::string& variantSetName) const;

  /// Stable-sorted union of authored attribute field names on \p prim_path across composed layers.
  std::vector<std::string> ListComposedFieldNames(const freeusd::sdf::Path& prim_path) const;

  /// Sorted union of time-sample keys for \p name on \p prim_path across composed layers.
  std::vector<double> ListComposedFieldSampleTimes(const freeusd::sdf::Path& prim_path,
                                                    const freeusd::tf::Token& name) const;

  /// Stable-sorted union of relationship names on \p prim_path across composed layers.
  std::vector<std::string> ListComposedRelationshipNames(const freeusd::sdf::Path& prim_path) const;

  /// Stable-sorted union of prim paths authored in any composed layer.
  std::vector<freeusd::sdf::Path> ListComposedPrimPaths() const;

  /// Layer \c relocates across the stack: strongest layer wins when the same source path is authored twice.
  bool GetComposedRelocateTarget(const freeusd::sdf::Path& fromPrimPath, freeusd::sdf::Path* outToPrimPath) const;
  bool RelocateSourceInAnyLayer(const freeusd::sdf::Path& fromPrimPath) const;
  /// All composed pairs sorted by source path (strongest opinion per source).
  std::vector<std::pair<freeusd::sdf::Path, freeusd::sdf::Path>> ListComposedRelocates() const;

  /// Layer \c prefixSubstitutions across the stack: strongest layer wins when the same source prefix is authored twice.
  bool GetComposedPrefixSubstitution(const std::string& fromPrefix, std::string* outToPrefix) const;
  bool PrefixSubstitutionKeyInAnyLayer(const std::string& fromPrefix) const;
  /// All composed pairs sorted by \p fromPrefix (strongest opinion per key).
  std::vector<std::pair<std::string, std::string>> ListComposedPrefixSubstitutions() const;

  /// \c defaultPrim on the root (strongest) layer.
  bool HasDefaultPrim() const;
  std::string GetDefaultPrimName() const;
  /// Prim at \c /{defaultPrim} when authored; invalid if none.
  Prim GetDefaultPrim() const;

  /// Pseudoroot hints composed strongest-first: use the first layer that authors a value (fallback when the root omits).
  std::optional<double> GetStartTimeCode() const;
  std::optional<double> GetEndTimeCode() const;
  std::optional<double> GetTimeCodesPerSecond() const;
  std::optional<double> GetFramesPerSecond() const;
  std::optional<int> GetFramePrecision() const;
  std::optional<double> GetMetersPerUnit() const;
  std::optional<std::string> GetUpAxis() const;
  std::vector<freeusd::sdf::Path> GetPrimOrder() const;

  freeusd::sdf::Path GetPseudoRootPath() const;

  Prim GetPrimAtPath(const freeusd::sdf::Path& path) const;
  std::vector<Prim> GetChildren(const freeusd::sdf::Path& primPath) const;

  /// Depth-first pre-order over composed prims under \c /. Visitor returns \c false to skip descending into that prim's subtree.
  void TraversePreorder(const std::function<bool(const Prim& prim)>& visitor) const;

  void SetResolver(std::unique_ptr<freeusd::ar::Resolver> resolver);

 private:
  explicit Stage(std::vector<std::shared_ptr<freeusd::sdf::Layer>> compose,
                 std::vector<freeusd::sdf::LayerOffset> compose_offsets = {});

  /// Maps \p authored through composed \c inherits / \c specializes arcs; stops when \p visitor returns true.
  bool VisitInternalArcMappedPrimPaths(const freeusd::sdf::Path& authored,
                                      const std::function<bool(const freeusd::sdf::Path& mapped_composed)>& visitor) const;

  std::vector<std::shared_ptr<freeusd::sdf::Layer>> compose_;
  std::vector<freeusd::sdf::LayerOffset> compose_offsets_;
  std::unique_ptr<freeusd::ar::Resolver> resolver_;
};

}  // namespace freeusd::usd
