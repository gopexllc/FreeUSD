#pragma once

#include <memory>
#include <string>
#include <vector>

#include "freeusd/export.hpp"
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

  bool HasAttribute(const freeusd::tf::Token& name) const;
  freeusd::vt::Value GetAttribute(const freeusd::tf::Token& name) const;

  bool HasRelationship(const freeusd::tf::Token& relName) const;
  std::vector<freeusd::sdf::Path> GetRelationshipTargets(const freeusd::tf::Token& relName) const;

  /// Composed USD schema type token (first authored kind on the layer stack wins).
  freeusd::tf::Token GetPrimKind() const;
  bool HasPrimKind() const;

  /// Effective authored **active**: strongest explicit opinion; default active if none.
  bool IsActive() const;
  bool HasPrimActiveOpinion() const;

  /// `customData`: true if any composed layer authors \p key for this prim's path.
  bool HasCustomDataKey(const std::string& key) const;
  /// Composed `customData` value for \p key (strongest opinion); empty if absent.
  freeusd::vt::Value GetCustomData(const std::string& key) const;
  std::vector<std::string> ListCustomDataKeys() const;

 private:
  std::shared_ptr<const Stage> lock_stage() const;
  std::weak_ptr<const Stage> stage_;
  freeusd::sdf::Path path_{};
};

}  // namespace freeusd::usd
