#pragma once

#include <memory>
#include <string>
#include <vector>

#include "freeusd/export.hpp"
#include "freeusd/sdf/layer.hpp"
#include "freeusd/sdf/path.hpp"
#include "freeusd/tf/token.hpp"
#include "freeusd/vt/value.hpp"

namespace freeusd::usd {

class Stage;

/// Handle to a prim on a stage (UsdPrim-shaped surface, clean-room).
class FREEUSD_API Prim {
 public:
  Prim() = default;
  Prim(std::weak_ptr<const Stage> stage, freeusd::sdf::Path path);

  bool IsValid() const noexcept;
  freeusd::sdf::Path GetPath() const noexcept { return path_; }

  /// Final path component (prim name); empty if not a prim path.
  std::string GetName() const;

  /// Owning stage, or null if the stage was destroyed.
  std::shared_ptr<const Stage> GetStage() const;

  /// Parent prim; invalid if this prim is the absolute root or not on a stage.
  Prim GetParent() const;

  /// Direct child prims (composed); empty if invalid.
  std::vector<Prim> GetChildren() const;

  bool HasAttribute(const freeusd::tf::Token& name) const;
  /// Evaluated attribute at \p time (hold interpolation; default stage time is \c 1.0 when omitted).
  freeusd::vt::Value GetAttribute(const freeusd::tf::Token& name, double time = 1.0) const;

  /// Composed attribute names (union across the layer stack, stable sort).
  std::vector<std::string> ListAttributeNames() const;
  /// Composed relationship names on this prim.
  std::vector<std::string> ListRelationshipNames() const;
  /// Union of time-sample times for \p name across composed layers (sorted).
  std::vector<double> ListAttributeSampleTimes(const freeusd::tf::Token& name) const;

  bool HasAttributeConnection(const freeusd::tf::Token& attr_name) const;
  /// Strongest composed \c .connect target for \p attr_name (full property path). False if none.
  bool GetAttributeConnectionTarget(const freeusd::tf::Token& attr_name, freeusd::sdf::Path* out_target) const;

  bool HasRelationship(const freeusd::tf::Token& relName) const;
  std::vector<freeusd::sdf::Path> GetRelationshipTargets(const freeusd::tf::Token& relName) const;

  std::vector<freeusd::sdf::PrimReference> GetReferences() const;
  bool HasReferences() const;
  std::vector<freeusd::sdf::Path> GetInherits() const;
  bool HasInherits() const;
  std::vector<freeusd::sdf::PrimReference> GetPayloads() const;
  bool HasPayloads() const;

  /// Composed USD schema type token (first authored kind on the layer stack wins).
  freeusd::tf::Token GetPrimKind() const;
  bool HasPrimKind() const;

  /// Composed prim specifier (\c def / \c class / \c over); \c Default means \c def-style.
  freeusd::sdf::Layer::PrimSpecifierKind GetSpecifierKind() const;
  /// True if composed specifier is \c class (Usd \c IsAbstract).
  bool IsAbstract() const;

  /// Effective authored **active**: strongest explicit opinion; default active if none.
  bool IsActive() const;
  bool HasPrimActiveOpinion() const;

  /// `customData`: true if any composed layer authors \p key for this prim's path.
  bool HasCustomDataKey(const std::string& key) const;
  /// Composed `customData` value for \p key (strongest opinion); empty if absent.
  freeusd::vt::Value GetCustomData(const std::string& key) const;
  std::vector<std::string> ListCustomDataKeys() const;

  /// Composed `variantSelection`: \c HasVariantSelectionKey mirrors any-layer presence (like \c HasCustomDataKey).
  bool HasVariantSelectionKey(const std::string& variantSet) const;
  /// Strongest composed selected variant name for \p variantSet; empty if none.
  std::string GetVariantSelection(const std::string& variantSet) const;
  std::vector<std::string> ListVariantSelectionSets() const;

  /// Composed \c variantSets: \c HasVariantSet is true if any layer declares the set.
  bool HasVariantSet(const std::string& variantSetName) const;
  std::vector<std::string> ListVariantSetNames() const;
  /// Strongest layer's variant name list for \p variantSetName (empty if the set is not declared).
  std::vector<std::string> ListVariantNames(const std::string& variantSetName) const;

 private:
  std::shared_ptr<const Stage> lock_stage() const;
  std::weak_ptr<const Stage> stage_;
  freeusd::sdf::Path path_{};
};

}  // namespace freeusd::usd
