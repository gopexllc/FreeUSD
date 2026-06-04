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

  const auto indices = mesh.GetFaceVertexIndices(1.0);
  assert(indices.size() == 3u);
  assert(indices[0] == 0 && indices[1] == 1 && indices[2] == 2);

  const auto colors = mesh.GetDisplayColor(1.0);
  assert(colors.size() == 3u);
  assert(near(colors[0].x(), 1.0f) && near(colors[0].y(), 0.0f));
  assert(near(colors[1].y(), 1.0f));
  assert(near(colors[2].z(), 1.0f));

  const auto normals = mesh.GetNormals(1.0);
  assert(normals.size() == 3u);
  for (const auto& n : normals) {
    assert(near(n.x(), 0.0f));
    assert(near(n.y(), 0.0f));
    assert(near(n.z(), 1.0f));
  }

  const auto st = mesh.GetPrimvarsSt(1.0);
  assert(st.size() == 3u);
  assert(near(st[0].s, 0.0f) && near(st[0].t, 0.0f));
  assert(near(st[1].s, 1.0f) && near(st[1].t, 0.0f));
  assert(near(st[2].s, 0.0f) && near(st[2].t, 1.0f));

  float opacity = 0.0f;
  assert(mesh.GetDisplayOpacity(&opacity, 1.0));
  assert(near(opacity, 0.75f));

  freeusd::gf::Vec3f ext_min{};
  freeusd::gf::Vec3f ext_max{};
  assert(mesh.GetExtent(&ext_min, &ext_max, 1.0));
  assert(near(ext_min.x(), 0.0f) && near(ext_min.y(), 0.0f) && near(ext_min.z(), 0.0f));
  assert(near(ext_max.x(), 1.0f) && near(ext_max.y(), 1.0f) && near(ext_max.z(), 0.0f));

  std::string scheme;
  assert(mesh.GetSubdivisionScheme(&scheme, 1.0));
  assert(scheme == "none");

  return 0;
}
