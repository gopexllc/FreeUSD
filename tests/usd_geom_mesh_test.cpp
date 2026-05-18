#include <cassert>
#include <cmath>
#include <string>

#include "freeusd/sdf/path.hpp"
#include "freeusd/usd/stage.hpp"
#include "freeusd/usdGeom/mesh.hpp"

#ifndef FREEUSD_TEST_FIXTURES_DIR
#error "FREEUSD_TEST_FIXTURES_DIR must be set by CMake for usd_geom_mesh_test"
#endif

namespace {

std::string fixture(const char* name) {
  return std::string(FREEUSD_TEST_FIXTURES_DIR) + "/" + name;
}

bool near(float a, float b) {
  return std::fabs(a - b) < 1e-5f;
}

}  // namespace

int main() {
  using freeusd::sdf::Path;
  using freeusd::usd::Stage;
  using freeusd::usdGeom::Mesh;

  std::string err;
  auto stage = Stage::OpenFromRootFile(fixture("parity_geom_mesh.usda"), freeusd::usd::RootLayerSublayersPolicy::None,
                                      &err);
  assert(stage && err.empty());

  const Path mesh_path = Path::FromString("/World/Tri");
  assert(stage->PrimPathInUse(mesh_path));
  Mesh mesh(stage->GetPrimAtPath(mesh_path));
  assert(mesh);

  const auto points = mesh.GetPoints(1.0);
  assert(points.size() == 3u);
  assert(near(points[0].x(), 0.0f) && near(points[0].y(), 0.0f) && near(points[0].z(), 0.0f));
  assert(near(points[1].x(), 1.0f) && near(points[1].y(), 0.0f));
  assert(near(points[2].x(), 0.0f) && near(points[2].y(), 1.0f));

  const auto counts = mesh.GetFaceVertexCounts(1.0);
  assert(counts.size() == 1u);
  assert(counts[0] == 3);

  return 0;
}
