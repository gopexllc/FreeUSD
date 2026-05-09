#pragma once

#include <memory>
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

class FREEUSD_API Stage : public std::enable_shared_from_this<Stage> {
 public:
  static std::shared_ptr<Stage> AttachRootLayer(std::shared_ptr<freeusd::sdf::Layer> root);

  /// Strongest layer is \c compose_layers()[0]; remainder are weaker (e.g. sublayers).
  static std::shared_ptr<Stage> AttachLayerStack(freeusd::pcp::LayerStack stack);

  const freeusd::sdf::Layer& GetRootLayer() const { return *compose_.front(); }
  std::shared_ptr<freeusd::sdf::Layer> GetRootLayerPtr() const {
    return compose_.empty() ? nullptr : compose_.front();
  }

  const std::vector<std::shared_ptr<freeusd::sdf::Layer>>& GetComposeLayers() const noexcept { return compose_; }

  /// Evaluated attribute at \p time (default + time samples per layer); strongest opinion wins.
  bool ReadFieldAtEvaluatedTime(const freeusd::sdf::Path& prim_path, const freeusd::tf::Token& name, double time,
                                freeusd::vt::Value* out) const;

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

  freeusd::tf::Token ResolvePrimKind(const freeusd::sdf::Path& prim_path) const;
  bool ResolveHasPrimKind(const freeusd::sdf::Path& prim_path) const;

  bool ResolvePrimActive(const freeusd::sdf::Path& prim_path) const;
  bool ResolveHasPrimActiveOpinion(const freeusd::sdf::Path& prim_path) const;

  /// Composed `customData` entry: strongest layer with an opinion wins.
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

  freeusd::sdf::Path GetPseudoRootPath() const;

  Prim GetPrimAtPath(const freeusd::sdf::Path& path) const;
  std::vector<Prim> GetChildren(const freeusd::sdf::Path& primPath) const;

  void SetResolver(std::unique_ptr<freeusd::ar::Resolver> resolver);

 private:
  explicit Stage(std::vector<std::shared_ptr<freeusd::sdf::Layer>> compose);

  std::vector<std::shared_ptr<freeusd::sdf::Layer>> compose_;
  std::unique_ptr<freeusd::ar::Resolver> resolver_;
};

}  // namespace freeusd::usd
