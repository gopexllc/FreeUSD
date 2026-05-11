#include "freeusd/usdGeom/boundable.hpp"

#include <algorithm>
#include <array>

#include "freeusd/gf/vec3d.hpp"
#include "freeusd/tf/token.hpp"
#include "freeusd/usdGeom/xformable.hpp"

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
  return freeusd::gf::BBox3d::Empty();
}

freeusd::gf::BBox3d Boundable::ComputeWorldBound(double time) const {
  const freeusd::gf::BBox3d local = ComputeLocalBound(time);
  if (local.IsEmpty()) {
    return local;
  }
  const freeusd::gf::Matrix4d xform = freeusd::usdGeom::Xformable(prim).ComputeLocalToWorldTransform(time);
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
  for (const freeusd::gf::Vec3d& p : corners) {
    min.set(std::min(min.x(), p.x()), std::min(min.y(), p.y()), std::min(min.z(), p.z()));
    max.set(std::max(max.x(), p.x()), std::max(max.y(), p.y()), std::max(max.z(), p.z()));
  }
  return freeusd::gf::BBox3d::FromMinMax(min, max);
}

}  // namespace freeusd::usdGeom
