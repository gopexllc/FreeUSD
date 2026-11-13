#include "freeusd/usdGeom/boundable.hpp"

#include <algorithm>
#include <array>

#include "freeusd/gf/vec3d.hpp"
#include "freeusd/tf/token.hpp"
#include "freeusd/usdGeom/mesh.hpp"
#include "freeusd/usdGeom/xformable.hpp"
#include "freeusd/vt/value.hpp"

namespace freeusd::usdGeom {
namespace {

freeusd::gf::Vec3d transform_point(const freeusd::gf::Matrix4d& m, const freeusd::gf::Vec3d& p) {
  const double* v = m.data();
  const double x = p.x() * v[0] + p.y() * v[4] + p.z() * v[8] + v[12];
  const double y = p.x() * v[1] + p.y() * v[5] + p.z() * v[9] + v[13];
  const double z = p.x() * v[2] + p.y() * v[6] + p.z() * v[10] + v[14];
  const double w = p.x() * v[3] + p.y() * v[7] + p.z() * v[11] + v[15];
  freeusd::gf::Vec3d out{};
  if (w != 0.0 && w != 1.0) {
    out.set(x / w, y / w, z / w);
  } else {
    out.set(x, y, z);
  }
  return out;
}

freeusd::gf::BBox3d bounds_from_half_extent(double hx, double hy, double hz) {
  freeusd::gf::Vec3d min{};
  freeusd::gf::Vec3d max{};
  min.set(-hx, -hy, -hz);
  max.set(hx, hy, hz);
  return freeusd::gf::BBox3d::FromMinMax(min, max);
}

freeusd::gf::BBox3d bounds_from_points(const std::vector<freeusd::gf::Vec3f>& points) {
  if (points.empty()) {
    return freeusd::gf::BBox3d::Empty();
  }
  freeusd::gf::Vec3d min{};
  freeusd::gf::Vec3d max{};
  min.set(points.front().x(), points.front().y(), points.front().z());
  max = min;
  for (const freeusd::gf::Vec3f& pt : points) {
    min.set(std::min(min.x(), static_cast<double>(pt.x())), std::min(min.y(), static_cast<double>(pt.y())),
           std::min(min.z(), static_cast<double>(pt.z())));
    max.set(std::max(max.x(), static_cast<double>(pt.x())), std::max(max.y(), static_cast<double>(pt.y())),
           std::max(max.z(), static_cast<double>(pt.z())));
  }
  return freeusd::gf::BBox3d::FromMinMax(min, max);
}

freeusd::gf::BBox3d bounds_from_extent_value(const freeusd::vt::Value& v) {
  std::vector<freeusd::gf::Vec3f> corners_f;
  if (v.GetVec3fArray(&corners_f) && corners_f.size() >= 2u) {
    freeusd::gf::Vec3d a{};
    freeusd::gf::Vec3d b{};
    a.set(corners_f[0].x(), corners_f[0].y(), corners_f[0].z());
    b.set(corners_f[1].x(), corners_f[1].y(), corners_f[1].z());
    return freeusd::gf::BBox3d::FromMinMax(a, b);
  }
  freeusd::gf::Vec3d corner{};
  if (v.GetVec3d(&corner)) {
    return freeusd::gf::BBox3d::FromMinMax(corner, corner);
  }
  freeusd::gf::Vec3f corner_f{};
  if (v.GetVec3f(&corner_f)) {
    freeusd::gf::Vec3d a{};
    a.set(corner_f.x(), corner_f.y(), corner_f.z());
    return freeusd::gf::BBox3d::FromMinMax(a, a);
  }
  return freeusd::gf::BBox3d::Empty();
}

freeusd::gf::BBox3d transform_local_bound(const freeusd::gf::BBox3d& local, const freeusd::gf::Matrix4d& xform) {
  if (local.IsEmpty()) {
    return local;
  }
  std::array<freeusd::gf::Vec3d, 8> corners{};
  corners[0] = transform_point(xform, local.min);
  freeusd::gf::Vec3d p{};
  p.set(local.max.x(), local.min.y(), local.min.z());
  corners[1] = transform_point(xform, p);
  p.set(local.min.x(), local.max.y(), local.min.z());
  corners[2] = transform_point(xform, p);
  p.set(local.min.x(), local.min.y(), local.max.z());
  corners[3] = transform_point(xform, p);
  p.set(local.max.x(), local.max.y(), local.min.z());
  corners[4] = transform_point(xform, p);
  p.set(local.max.x(), local.min.y(), local.max.z());
  corners[5] = transform_point(xform, p);
  p.set(local.min.x(), local.max.y(), local.max.z());
  corners[6] = transform_point(xform, p);
  corners[7] = transform_point(xform, local.max);
  freeusd::gf::Vec3d min = corners[0];
  freeusd::gf::Vec3d max = corners[0];
  for (const freeusd::gf::Vec3d& corner : corners) {
    min.set(std::min(min.x(), corner.x()), std::min(min.y(), corner.y()), std::min(min.z(), corner.z()));
    max.set(std::max(max.x(), corner.x()), std::max(max.y(), corner.y()), std::max(max.z(), corner.z()));
  }
  return freeusd::gf::BBox3d::FromMinMax(min, max);
}

freeusd::gf::BBox3d union_child_world_bounds(const freeusd::usd::Prim& prim, double time) {
  freeusd::gf::BBox3d aggregate = freeusd::gf::BBox3d::Empty();
  for (const freeusd::usd::Prim& child : prim.GetChildren()) {
    if (!child.IsValid() || !child.IsActive()) {
      continue;
    }
    const freeusd::gf::BBox3d child_world = Boundable(child).ComputeWorldBound(time);
    aggregate = freeusd::gf::BBox3d::Union(aggregate, child_world);
  }
  return aggregate;
}

}  // namespace

freeusd::gf::BBox3d Boundable::ComputeLocalBound(double time) const {
  if (!prim.IsValid()) {
    return freeusd::gf::BBox3d::Empty();
  }
  if (auto size = prim.GetAttribute(freeusd::tf::Token("size"), time); !size.IsEmpty()) {
    double d = 0.0;
    float f = 0.0F;
    if (size.GetDouble(&d) || (size.GetFloat(&f) && (d = static_cast<double>(f), true))) {
      return bounds_from_half_extent(d * 0.5, d * 0.5, d * 0.5);
    }
  }
  if (auto radius = prim.GetAttribute(freeusd::tf::Token("radius"), time); !radius.IsEmpty()) {
    double r = 0.0;
    float f = 0.0F;
    if (radius.GetDouble(&r) || (radius.GetFloat(&f) && (r = static_cast<double>(f), true))) {
      return bounds_from_half_extent(r, r, r);
    }
  }
  if (auto extent = prim.GetAttribute(freeusd::tf::Token("extent"), time); !extent.IsEmpty()) {
    const freeusd::gf::BBox3d from_extent = bounds_from_extent_value(extent);
    if (!from_extent.IsEmpty()) {
      return from_extent;
    }
  }
  const Mesh mesh(prim);
  const std::vector<freeusd::gf::Vec3f> points = mesh.GetPoints(time);
  if (!points.empty()) {
    return bounds_from_points(points);
  }
  return freeusd::gf::BBox3d::Empty();
}

freeusd::gf::BBox3d Boundable::ComputeWorldBound(double time) const {
  if (!prim.IsValid()) {
    return freeusd::gf::BBox3d::Empty();
  }
  const freeusd::gf::BBox3d local = ComputeLocalBound(time);
  if (!local.IsEmpty()) {
    const freeusd::gf::Matrix4d xform = freeusd::usdGeom::Xformable(prim).ComputeLocalToWorldTransform(time);
    return transform_local_bound(local, xform);
  }
  return union_child_world_bounds(prim, time);
}

}  // namespace freeusd::usdGeom
