#pragma once

#include <memory>
#include <vector>

#include "freeusd/ar/defaultResolver.hpp"
#include "freeusd/export.hpp"
#include "freeusd/sdf/layer.hpp"
#include "freeusd/usd/prim.hpp"

namespace freeusd::usd {

class FREEUSD_API Stage : public std::enable_shared_from_this<Stage> {
 public:
  static std::shared_ptr<Stage> AttachRootLayer(std::shared_ptr<freeusd::sdf::Layer> root);

  const freeusd::sdf::Layer& GetRootLayer() const { return *root_; }
  std::shared_ptr<freeusd::sdf::Layer> GetRootLayerPtr() const { return root_; }

  freeusd::sdf::Path GetPseudoRootPath() const;

  Prim GetPrimAtPath(const freeusd::sdf::Path& path) const;
  std::vector<Prim> GetChildren(const freeusd::sdf::Path& primPath) const;

  void SetResolver(std::unique_ptr<freeusd::ar::Resolver> resolver);

 private:
  explicit Stage(std::shared_ptr<freeusd::sdf::Layer> root);

  std::shared_ptr<freeusd::sdf::Layer> root_;
  std::unique_ptr<freeusd::ar::Resolver> resolver_;
};

}  // namespace freeusd::usd
