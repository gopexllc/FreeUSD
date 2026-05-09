#pragma once

#include <memory>
#include <string>
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
