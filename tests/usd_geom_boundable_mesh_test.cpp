#include <cassert>
#include <cmath>
#include <string>

#include "freeusd/sdf/path.hpp"
#include "freeusd/usd/stage.hpp"
#include "freeusd/usdGeom/boundable.hpp"
#include "freeusd/usdUtils/engineScene.hpp"

#ifndef FREEUSD_TEST_FIXTURES_DIR
#error "FREEUSD_TEST_FIXTURES_DIR must be set by CMake for usd_geom_boundable_mesh_test"
#endif

namespace {

std::string fixture(const char* name) {
  return std::string(FREEUSD_TEST_FIXTURES_DIR) + "/" + name;
}

bool near3(double a, double b) {
  return std::fabs(a - b) < 1e-5;
}

bool bbox_near(const freeusd::gf::BBox3d& box, double minx, double miny, double minz, double maxx, double maxy,
               double maxz) {
  return near3(box.min.x(), minx) && near3(box.min.y(), miny) && near3(box.min.z(), minz) &&
         near3(box.max.x(), maxx) && near3(box.max.y(), maxy) && near3(box.max.z(), maxz);
}

}  // namespace

int main() {
  using freeusd::sdf::Path;
  using freeusd::usd::Stage;
  using freeusd::usdGeom::Boundable;
  using freeusd::usdUtils::BuildEngineSceneSnapshot;

  std::string err;
  auto stage = Stage::OpenFromRootFile(fixture("parity_boundable_mesh.usda"),
                                      freeusd::usd::RootLayerSublayersPolicy::None, &err);
  assert(stage && err.empty());

  const Path mesh_path = Path::FromString("/World/Group/Quad");
  const Path group_path = Path::FromString("/World/Group");

  Boundable mesh(stage->GetPrimAtPath(mesh_path));
  assert(mesh);
  assert(bbox_near(mesh.ComputeLocalBound(1.0), 0.0, 0.0, 0.0, 4.0, 3.0, 0.0));
  assert(bbox_near(mesh.ComputeWorldBound(1.0), 10.0, 20.0, 30.0, 14.0, 23.0, 30.0));

  Boundable group(stage->GetPrimAtPath(group_path));
  assert(group);
  assert(group.ComputeLocalBound(1.0).IsEmpty());
  assert(bbox_near(group.ComputeWorldBound(1.0), 10.0, 20.0, 30.0, 14.0, 23.0, 30.0));

  const auto snap = BuildEngineSceneSnapshot(*stage, 1.0);
  bool found_group = false;
  for (const auto& node : snap.nodes) {
    if (node.path == group_path) {
      found_group = true;
      assert(bbox_near(node.world_bound, 10.0, 20.0, 30.0, 14.0, 23.0, 30.0));
      break;
    }
  }
  assert(found_group);

  auto tri_stage = Stage::OpenFromRootFile(fixture("parity_geom_mesh.usda"), freeusd::usd::RootLayerSublayersPolicy::None,
                                          &err);
  assert(tri_stage && err.empty());
  Boundable tri(tri_stage->GetPrimAtPath(Path::FromString("/World/Tri")));
  assert(tri);
  assert(bbox_near(tri.ComputeWorldBound(1.0), 0.0, 0.0, 0.0, 1.0, 1.0, 0.0));

  return 0;
}
