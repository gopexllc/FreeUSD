#include "freeusd/usdVol/volume.hpp"

#include "freeusd/usd/stage.hpp"
#include "freeusd/usdVol/tokens.hpp"

namespace freeusd::usdVol {
namespace {

freeusd::tf::Token field_relationship_name() { return freeusd::tf::Token("field"); }

}  // namespace

Volume Volume::ReadFromPrim(const std::shared_ptr<const freeusd::usd::Stage>& stage,
                           const freeusd::sdf::Path& path) {
  if (!stage) {
    return {};
  }
  return Volume(stage->GetPrimAtPath(path));
}

bool Volume::IsVolume() const {
  return prim.IsValid() && prim.HasPrimKind() && prim.GetPrimKind() == tokens::Volume();
}

std::vector<freeusd::sdf::Path> Volume::GetFieldRelationshipTargets() const {
  if (!IsVolume()) {
    return {};
  }
  return prim.GetRelationshipTargets(field_relationship_name());
}

std::vector<OpenVDBAsset> Volume::GetOpenVDBFieldAssets() const {
  std::vector<OpenVDBAsset> out;
  if (!IsVolume()) {
    return out;
  }
  const std::shared_ptr<const freeusd::usd::Stage> stage = prim.GetStage();
  if (!stage) {
    return out;
  }
  for (const freeusd::sdf::Path& target : GetFieldRelationshipTargets()) {
    if (target.IsEmpty()) {
      continue;
    }
    const OpenVDBAsset field = OpenVDBAsset::ReadFromPrim(stage, target);
    if (field.IsOpenVDBAsset()) {
      out.push_back(field);
    }
  }
  return out;
}

}  // namespace freeusd::usdVol
