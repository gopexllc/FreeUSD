#include <pybind11/functional.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <cstdint>
#include <memory>
#include <optional>
#include <sstream>

#include "freeusd/ar/defaultResolver.hpp"
#include "freeusd/ar/resolvedPath.hpp"
#include "freeusd/gf/bbox3d.hpp"
#include "freeusd/gf/matrix4d.hpp"
#include "freeusd/gf/quatd.hpp"
#include "freeusd/gf/quatf.hpp"
#include "freeusd/gf/range1d.hpp"
#include "freeusd/gf/vec3d.hpp"
#include "freeusd/gf/vec3f.hpp"
#include "freeusd/io/usda.hpp"
#include "freeusd/pcp/compose.hpp"
#include "freeusd/pcp/layerStack.hpp"
#include "freeusd/plug/registry.hpp"
#include "freeusd/sdf/assetPath.hpp"
#include "freeusd/sdf/layer.hpp"
#include "freeusd/sdf/layerOffset.hpp"
#include "freeusd/sdf/path.hpp"
#include "freeusd/sdf/primReference.hpp"
#include "freeusd/sdf/tokens.hpp"
#include "freeusd/tf/token.hpp"
#include "freeusd/trace/collector.hpp"
#include "freeusd/usd/crateFile.hpp"
#include "freeusd/usd/editTarget.hpp"
#include "freeusd/usd/kindTokens.hpp"
#include "freeusd/usd/schemaDataTokens.hpp"
#include "freeusd/usd/tokens.hpp"
#include "freeusd/usd/stage.hpp"
#include "freeusd/usd/timeCode.hpp"
#include "freeusd/usdUtils/pipeline.hpp"
#include "freeusd/usdGeom/boundable.hpp"
#include "freeusd/usdGeom/imageable.hpp"
#include "freeusd/usdGeom/tokens.hpp"
#include "freeusd/usdGeom/xformable.hpp"
#include "freeusd/usdHydra/tokens.hpp"
#include "freeusd/usdLux/tokens.hpp"
#include "freeusd/usdMedia/tokens.hpp"
#include "freeusd/usdMtlx/tokens.hpp"
#include "freeusd/usdPhysics/tokens.hpp"
#include "freeusd/usdProc/tokens.hpp"
#include "freeusd/usdRender/tokens.hpp"
#include "freeusd/usdRi/tokens.hpp"
#include "freeusd/usdSemantics/tokens.hpp"
#include "freeusd/usdShade/tokens.hpp"
#include "freeusd/usdSkel/tokens.hpp"
#include "freeusd/usdUI/tokens.hpp"
#include "freeusd/usdVol/tokens.hpp"
#include "freeusd/version.hpp"
#include "freeusd/vt/value.hpp"
#include "freeusd/work/dispatcher.hpp"

namespace py = pybind11;

/// Composed attribute reads at \p time (shared by `Stage.read_field_*` and `Prim.read_field_*`).
namespace freeusd_py_composed_read {

bool read_field_value(const freeusd::usd::Stage& st, const freeusd::sdf::Path& p, const freeusd::tf::Token& t,
                      double time, freeusd::vt::Value* out) {
  return out && st.ReadFieldAtEvaluatedTime(p, t, time, out);
}

bool value_to_float(const freeusd::vt::Value& v, float* out) {
  if (!out) {
    return false;
  }
  if (v.GetFloat(out)) {
    return true;
  }
  double d = 0.0;
  if (v.GetDouble(&d)) {
    *out = static_cast<float>(d);
    return true;
  }
  std::int32_t i32 = 0;
  if (v.GetInt32(&i32)) {
    *out = static_cast<float>(i32);
    return true;
  }
  std::int64_t i64 = 0;
  if (v.GetInt64(&i64)) {
    *out = static_cast<float>(i64);
    return true;
  }
  bool b = false;
  if (v.GetBool(&b)) {
    *out = b ? 1.0F : 0.0F;
    return true;
  }
  return false;
}

bool value_to_int64(const freeusd::vt::Value& v, std::int64_t* out) {
  if (!out) {
    return false;
  }
  bool b = false;
  if (v.GetBool(&b)) {
    *out = b ? 1 : 0;
    return true;
  }
  std::int32_t i32 = 0;
  if (v.GetInt32(&i32)) {
    *out = i32;
    return true;
  }
  if (v.GetInt64(out)) {
    return true;
  }
  float f = 0.0F;
  if (v.GetFloat(&f)) {
    *out = static_cast<std::int64_t>(f);
    return true;
  }
  double d = 0.0;
  if (v.GetDouble(&d)) {
    *out = static_cast<std::int64_t>(d);
    return true;
  }
  return false;
}

bool value_to_string(const freeusd::vt::Value& v, std::string* out) {
  if (!out) {
    return false;
  }
  if (v.GetString(out)) {
    return true;
  }
  freeusd::tf::Token tok;
  if (v.GetToken(&tok)) {
    *out = tok.GetText();
    return true;
  }
  return false;
}

bool value_to_vec3f(const freeusd::vt::Value& v, freeusd::gf::Vec3f* out) {
  if (!out) {
    return false;
  }
  if (v.GetVec3f(out)) {
    return true;
  }
  freeusd::gf::Vec3d vd{};
  if (v.GetVec3d(&vd)) {
    out->set(static_cast<float>(vd.x()), static_cast<float>(vd.y()), static_cast<float>(vd.z()));
    return true;
  }
  return false;
}

bool value_to_quatf(const freeusd::vt::Value& v, freeusd::gf::Quatf* out) {
  if (!out) {
    return false;
  }
  if (v.GetQuatf(out)) {
    return true;
  }
  freeusd::gf::Quatd qd{};
  if (v.GetQuatd(&qd)) {
    *out = freeusd::gf::Quatf(static_cast<float>(qd.real), static_cast<float>(qd.i), static_cast<float>(qd.j),
                              static_cast<float>(qd.k));
    return true;
  }
  return false;
}

py::object read_field_at_time(const freeusd::usd::Stage& st, const freeusd::sdf::Path& p,
                              const freeusd::tf::Token& t, double time) {
  freeusd::vt::Value v;
  if (read_field_value(st, p, t, time, &v)) {
    return py::cast(v);
  }
  return py::none();
}

py::object read_field_double(const freeusd::usd::Stage& st, const freeusd::sdf::Path& p,
                              const freeusd::tf::Token& t, double time) {
  freeusd::vt::Value v;
  if (!read_field_value(st, p, t, time, &v)) {
    return py::none();
  }
  double d = 0.0;
  if (v.GetDouble(&d)) {
    return py::cast(d);
  }
  return py::none();
}

py::object read_field_float(const freeusd::usd::Stage& st, const freeusd::sdf::Path& p,
                            const freeusd::tf::Token& t, double time) {
  freeusd::vt::Value v;
  if (!read_field_value(st, p, t, time, &v)) {
    return py::none();
  }
  float f = 0.0F;
  if (value_to_float(v, &f)) {
    return py::cast(f);
  }
  return py::none();
}

py::object read_field_bool(const freeusd::usd::Stage& st, const freeusd::sdf::Path& p, const freeusd::tf::Token& t,
                           double time) {
  freeusd::vt::Value v;
  if (!read_field_value(st, p, t, time, &v)) {
    return py::none();
  }
  bool b = false;
  if (v.GetBool(&b)) {
    return py::cast(b);
  }
  return py::none();
}

py::object read_field_int64(const freeusd::usd::Stage& st, const freeusd::sdf::Path& p, const freeusd::tf::Token& t,
                            double time) {
  freeusd::vt::Value v;
  if (!read_field_value(st, p, t, time, &v)) {
    return py::none();
  }
  std::int64_t n = 0;
  if (value_to_int64(v, &n)) {
    return py::cast(n);
  }
  return py::none();
}

py::object read_field_string(const freeusd::usd::Stage& st, const freeusd::sdf::Path& p, const freeusd::tf::Token& t,
                             double time) {
  freeusd::vt::Value v;
  if (!read_field_value(st, p, t, time, &v)) {
    return py::none();
  }
  std::string s;
  if (value_to_string(v, &s)) {
    return py::cast(s);
  }
  return py::none();
}

py::object read_field_vec3d(const freeusd::usd::Stage& st, const freeusd::sdf::Path& p, const freeusd::tf::Token& t,
                            double time) {
  freeusd::vt::Value v;
  if (!read_field_value(st, p, t, time, &v)) {
    return py::none();
  }
  freeusd::gf::Vec3d vec{};
  if (!v.GetVec3d(&vec)) {
    return py::none();
  }
  return py::make_tuple(vec.x(), vec.y(), vec.z());
}

py::object read_field_vec3f(const freeusd::usd::Stage& st, const freeusd::sdf::Path& p, const freeusd::tf::Token& t,
                            double time) {
  freeusd::vt::Value v;
  if (!read_field_value(st, p, t, time, &v)) {
    return py::none();
  }
  freeusd::gf::Vec3f vf{};
  if (value_to_vec3f(v, &vf)) {
    return py::make_tuple(static_cast<double>(vf.x()), static_cast<double>(vf.y()), static_cast<double>(vf.z()));
  }
  return py::none();
}

py::object read_field_matrix4d(const freeusd::usd::Stage& st, const freeusd::sdf::Path& p, const freeusd::tf::Token& t,
                               double time) {
  freeusd::vt::Value v;
  if (!read_field_value(st, p, t, time, &v)) {
    return py::none();
  }
  freeusd::gf::Matrix4d m{};
  if (!v.GetMatrix4d(&m)) {
    return py::none();
  }
  return py::cast(m);
}

py::object read_field_quatd(const freeusd::usd::Stage& st, const freeusd::sdf::Path& p, const freeusd::tf::Token& t,
                            double time) {
  freeusd::vt::Value v;
  if (!read_field_value(st, p, t, time, &v)) {
    return py::none();
  }
  freeusd::gf::Quatd q{};
  if (!v.GetQuatd(&q)) {
    return py::none();
  }
  return py::cast(q);
}

py::object read_field_quatf(const freeusd::usd::Stage& st, const freeusd::sdf::Path& p, const freeusd::tf::Token& t,
                            double time) {
  freeusd::vt::Value v;
  if (!read_field_value(st, p, t, time, &v)) {
    return py::none();
  }
  freeusd::gf::Quatf q{};
  if (!value_to_quatf(v, &q)) {
    return py::none();
  }
  return py::cast(q);
}

py::object read_field_token(const freeusd::usd::Stage& st, const freeusd::sdf::Path& p, const freeusd::tf::Token& t,
                            double time) {
  freeusd::vt::Value v;
  if (!read_field_value(st, p, t, time, &v)) {
    return py::none();
  }
  freeusd::tf::Token tok;
  if (!v.GetToken(&tok)) {
    return py::none();
  }
  return py::cast(tok);
}

py::object read_field_token_array(const freeusd::usd::Stage& st, const freeusd::sdf::Path& p,
                                  const freeusd::tf::Token& t, double time) {
  freeusd::vt::Value v;
  if (!read_field_value(st, p, t, time, &v)) {
    return py::none();
  }
  std::vector<freeusd::tf::Token> elems;
  if (!v.GetTokenArray(&elems)) {
    return py::none();
  }
  py::list lst;
  for (const freeusd::tf::Token& x : elems) {
    lst.append(py::cast(x));
  }
  return lst;
}

}  // namespace freeusd_py_composed_read

PYBIND11_MODULE(_native, m) {
  m.doc() = "FreeUSD native extension: modular OpenUSD-shaped runtime (clean-room).";

  m.def("version_string", [] { return freeusd::version_string(); });
  m.def("version_parts", [] {
    std::uint32_t maj{}, min{}, pat{};
    freeusd::version_parts(maj, min, pat);
    return py::make_tuple(maj, min, pat);
  });

  {
    auto tf = m.def_submodule("tf");
    py::class_<freeusd::tf::Token>(tf, "Token")
        .def(py::init<>())
        .def(py::init<std::string>())
        .def("is_empty", &freeusd::tf::Token::IsEmpty)
        .def("text", &freeusd::tf::Token::GetText)
        .def("__repr__", [](const freeusd::tf::Token& t) { return "<freeusd.tf.Token '" + t.GetText() + "'>"; });
  }

  {
    auto gf = m.def_submodule("gf");
    py::class_<freeusd::gf::BBox3d>(gf, "BBox3d")
        .def(py::init<>())
        .def_readwrite("min", &freeusd::gf::BBox3d::min)
        .def_readwrite("max", &freeusd::gf::BBox3d::max)
        .def("is_empty", &freeusd::gf::BBox3d::IsEmpty)
        .def_static("empty", &freeusd::gf::BBox3d::Empty)
        .def_static(
            "from_min_max",
            [](const freeusd::gf::Vec3d& a, const freeusd::gf::Vec3d& b) {
              return freeusd::gf::BBox3d::FromMinMax(a, b);
            },
            py::arg("a"),
            py::arg("b"))
        .def("__eq__", [](const freeusd::gf::BBox3d& u, const freeusd::gf::BBox3d& v) { return u == v; })
        .def("__repr__", [](const freeusd::gf::BBox3d&) { return "<freeusd.gf.BBox3d>"; });

    py::class_<freeusd::gf::Vec3d>(gf, "Vec3d")
        .def(py::init<>())
        .def(py::init<double, double, double>(), py::arg("x") = 0.0, py::arg("y") = 0.0, py::arg("z") = 0.0)
        .def_static("Zero", &freeusd::gf::Vec3d::Zero)
        .def("x", &freeusd::gf::Vec3d::x)
        .def("y", &freeusd::gf::Vec3d::y)
        .def("z", &freeusd::gf::Vec3d::z)
        .def("set", [](freeusd::gf::Vec3d& v, double x, double y, double z) { v.set(x, y, z); })
        .def("as_array", &freeusd::gf::Vec3d::as_array)
        .def("__eq__", [](const freeusd::gf::Vec3d& a, const freeusd::gf::Vec3d& b) { return a == b; })
        .def("__repr__", [](const freeusd::gf::Vec3d& v) {
          return "<freeusd.gf.Vec3d " + std::to_string(v.x()) + ", " + std::to_string(v.y()) + ", " +
                 std::to_string(v.z()) + ">";
        });

    py::class_<freeusd::gf::Vec3f>(gf, "Vec3f")
        .def(py::init<>())
        .def(py::init<float, float, float>(), py::arg("x") = 0.0F, py::arg("y") = 0.0F, py::arg("z") = 0.0F)
        .def_static("Zero", &freeusd::gf::Vec3f::Zero)
        .def("x", &freeusd::gf::Vec3f::x)
        .def("y", &freeusd::gf::Vec3f::y)
        .def("z", &freeusd::gf::Vec3f::z)
        .def("set", [](freeusd::gf::Vec3f& v, float x, float y, float z) { v.set(x, y, z); })
        .def("as_array", &freeusd::gf::Vec3f::as_array)
        .def("__eq__", [](const freeusd::gf::Vec3f& a, const freeusd::gf::Vec3f& b) { return a == b; })
        .def("__repr__", [](const freeusd::gf::Vec3f& v) {
          return "<freeusd.gf.Vec3f " + std::to_string(v.x()) + ", " + std::to_string(v.y()) + ", " +
                 std::to_string(v.z()) + ">";
        });

    py::class_<freeusd::gf::Matrix4d>(gf, "Matrix4d")
        .def(py::init<>())
        .def_static("Identity", &freeusd::gf::Matrix4d::Identity)
        .def_static(
            "Translate",
            static_cast<freeusd::gf::Matrix4d (*)(double, double, double)>(&freeusd::gf::Matrix4d::Translate),
            py::arg("tx"),
            py::arg("ty"),
            py::arg("tz"))
        .def_static("Shear", &freeusd::gf::Matrix4d::Shear, py::arg("h_xy"), py::arg("h_xz"), py::arg("h_yz"))
        .def_static("Scale", &freeusd::gf::Matrix4d::Scale, py::arg("sx"), py::arg("sy"), py::arg("sz"))
        .def_static("RotateDegreesX", &freeusd::gf::Matrix4d::RotateDegreesX, py::arg("degrees"))
        .def_static("RotateDegreesY", &freeusd::gf::Matrix4d::RotateDegreesY, py::arg("degrees"))
        .def_static("RotateDegreesZ", &freeusd::gf::Matrix4d::RotateDegreesZ, py::arg("degrees"))
        .def_static("RotateDegreesXYZ", &freeusd::gf::Matrix4d::RotateDegreesXYZ, py::arg("ax"), py::arg("ay"), py::arg("az"))
        .def_static("RotateDegreesXZY", &freeusd::gf::Matrix4d::RotateDegreesXZY, py::arg("ax"), py::arg("ay"), py::arg("az"))
        .def_static("RotateDegreesYXZ", &freeusd::gf::Matrix4d::RotateDegreesYXZ, py::arg("ax"), py::arg("ay"), py::arg("az"))
        .def_static("RotateDegreesYZX", &freeusd::gf::Matrix4d::RotateDegreesYZX, py::arg("ax"), py::arg("ay"), py::arg("az"))
        .def_static("RotateDegreesZXY", &freeusd::gf::Matrix4d::RotateDegreesZXY, py::arg("ax"), py::arg("ay"), py::arg("az"))
        .def_static("RotateDegreesZYX", &freeusd::gf::Matrix4d::RotateDegreesZYX, py::arg("ax"), py::arg("ay"), py::arg("az"))
        .def_static("FromUnitQuaternion", &freeusd::gf::Matrix4d::FromUnitQuaternion, py::arg("q"))
        .def_static("Multiply", &freeusd::gf::Matrix4d::Multiply, py::arg("a"), py::arg("b"))
        .def(
            "get_inverse",
            [](const freeusd::gf::Matrix4d& m) -> py::object {
              freeusd::gf::Matrix4d inv;
              if (m.GetInverse(&inv)) {
                return py::cast(inv);
              }
              return py::none();
            },
            "Returns inverse matrix or None if singular.")
        .def(
            "as_list",
            [](const freeusd::gf::Matrix4d& m) {
              return std::vector<double>(m.m.begin(), m.m.end());
            },
            "Row-major 16 doubles (host-defined layout; matches C++ `m` storage).")
        .def("__eq__", [](const freeusd::gf::Matrix4d& a, const freeusd::gf::Matrix4d& b) { return a == b; })
        .def("__repr__", [](const freeusd::gf::Matrix4d&) { return "<freeusd.gf.Matrix4d>"; });

    py::class_<freeusd::gf::Quatd>(gf, "Quatd")
        .def(py::init<>())
        .def(py::init<double, double, double, double>(), py::arg("real"), py::arg("i"), py::arg("j"), py::arg("k"))
        .def("normalized", &freeusd::gf::Quatd::Normalized)
        .def_readwrite("real", &freeusd::gf::Quatd::real)
        .def_readwrite("i", &freeusd::gf::Quatd::i)
        .def_readwrite("j", &freeusd::gf::Quatd::j)
        .def_readwrite("k", &freeusd::gf::Quatd::k)
        .def_static("Identity", &freeusd::gf::Quatd::Identity)
        .def("__eq__", [](const freeusd::gf::Quatd& a, const freeusd::gf::Quatd& b) { return a == b; })
        .def("__repr__", [](const freeusd::gf::Quatd& q) {
          return std::string{"<freeusd.gf.Quatd real="} + std::to_string(q.real) + " i=" + std::to_string(q.i) +
                 " j=" + std::to_string(q.j) + " k=" + std::to_string(q.k) + ">";
        });

    py::class_<freeusd::gf::Quatf>(gf, "Quatf")
        .def(py::init<>())
        .def(py::init<float, float, float, float>(), py::arg("real"), py::arg("i"), py::arg("j"), py::arg("k"))
        .def("normalized", &freeusd::gf::Quatf::Normalized)
        .def_readwrite("real", &freeusd::gf::Quatf::real)
        .def_readwrite("i", &freeusd::gf::Quatf::i)
        .def_readwrite("j", &freeusd::gf::Quatf::j)
        .def_readwrite("k", &freeusd::gf::Quatf::k)
        .def_static("Identity", &freeusd::gf::Quatf::Identity)
        .def("__eq__", [](const freeusd::gf::Quatf& a, const freeusd::gf::Quatf& b) { return a == b; })
        .def("__repr__", [](const freeusd::gf::Quatf& q) {
          return std::string{"<freeusd.gf.Quatf real="} + std::to_string(q.real) + " i=" + std::to_string(q.i) +
                 " j=" + std::to_string(q.j) + " k=" + std::to_string(q.k) + ">";
        });

    py::class_<freeusd::gf::Range1d>(gf, "Range1d")
        .def(py::init<>())
        .def(py::init<double, double>(), py::arg("min"), py::arg("max"))
        .def_readwrite("min", &freeusd::gf::Range1d::min)
        .def_readwrite("max", &freeusd::gf::Range1d::max)
        .def("is_empty", &freeusd::gf::Range1d::IsEmpty)
        .def_static("unit_interval", &freeusd::gf::Range1d::UnitInterval)
        .def("__eq__", [](const freeusd::gf::Range1d& a, const freeusd::gf::Range1d& b) { return a == b; })
        .def("__repr__", [](const freeusd::gf::Range1d& r) {
          return std::string{"<freeusd.gf.Range1d min="} + std::to_string(r.min) + " max=" + std::to_string(r.max) + ">";
        });
  }

  {
    auto vt = m.def_submodule("vt");
    py::class_<freeusd::vt::Value>(vt, "Value")
        .def(py::init<>())
        .def_static("make_bool", &freeusd::vt::Value::MakeBool)
        .def_static("make_int32", &freeusd::vt::Value::MakeInt32)
        .def_static("make_int64", &freeusd::vt::Value::MakeInt64)
        .def_static("make_float", &freeusd::vt::Value::MakeFloat)
        .def_static("make_double", &freeusd::vt::Value::MakeDouble)
        .def_static("make_string", &freeusd::vt::Value::MakeString)
        .def_static("make_token", &freeusd::vt::Value::MakeToken)
        .def_static(
            "make_vec3d",
            [](double x, double y, double z) {
              freeusd::gf::Vec3d v;
              v.set(x, y, z);
              return freeusd::vt::Value::MakeVec3d(v);
            },
            py::arg("x"),
            py::arg("y"),
            py::arg("z"))
        .def_static("make_vec3d_vec", [](const freeusd::gf::Vec3d& v) { return freeusd::vt::Value::MakeVec3d(v); }, py::arg("v"))
        .def_static(
            "make_vec3f",
            [](float x, float y, float z) {
              freeusd::gf::Vec3f v;
              v.set(x, y, z);
              return freeusd::vt::Value::MakeVec3f(v);
            },
            py::arg("x"),
            py::arg("y"),
            py::arg("z"))
        .def_static("make_vec3f_vec", [](const freeusd::gf::Vec3f& v) { return freeusd::vt::Value::MakeVec3f(v); }, py::arg("v"))
        .def_static("make_matrix4d", [](const freeusd::gf::Matrix4d& m) { return freeusd::vt::Value::MakeMatrix4d(m); }, py::arg("m"))
        .def_static(
            "make_quatd",
            [](double w, double i, double j, double k) { return freeusd::vt::Value::MakeQuatd(freeusd::gf::Quatd{w, i, j, k}); },
            py::arg("real"),
            py::arg("i"),
            py::arg("j"),
            py::arg("k"))
        .def_static(
            "make_quatf",
            [](float w, float i, float j, float k) { return freeusd::vt::Value::MakeQuatf(freeusd::gf::Quatf{w, i, j, k}); },
            py::arg("real"),
            py::arg("i"),
            py::arg("j"),
            py::arg("k"))
        .def_static(
            "make_token_array",
            [](const std::vector<std::string>& items) {
              std::vector<freeusd::tf::Token> v;
              v.reserve(items.size());
              for (const std::string& s : items) {
                v.emplace_back(freeusd::tf::Token{s});
              }
              return freeusd::vt::Value::MakeTokenArray(std::move(v));
            },
            py::arg("tokens"))
        .def("is_empty", &freeusd::vt::Value::IsEmpty)
        .def("holds_bool", &freeusd::vt::Value::HoldsBool)
        .def("holds_int32", &freeusd::vt::Value::HoldsInt32)
        .def("holds_int64", &freeusd::vt::Value::HoldsInt64)
        .def("holds_float", &freeusd::vt::Value::HoldsFloat)
        .def("holds_double", &freeusd::vt::Value::HoldsDouble)
        .def("holds_string", &freeusd::vt::Value::HoldsString)
        .def("holds_token", &freeusd::vt::Value::HoldsToken)
        .def("holds_token_array", &freeusd::vt::Value::HoldsTokenArray)
        .def("holds_quatd", &freeusd::vt::Value::HoldsQuatd)
        .def("holds_quatf", &freeusd::vt::Value::HoldsQuatf)
        .def("holds_matrix4d", &freeusd::vt::Value::HoldsMatrix4d)
        .def("holds_vec3d", &freeusd::vt::Value::HoldsVec3d)
        .def("holds_vec3f", &freeusd::vt::Value::HoldsVec3f)
        .def("as_bool", [](const freeusd::vt::Value& v) -> py::object {
          bool b{};
          if (v.GetBool(&b)) {
            return py::cast(b);
          }
          return py::none();
        })
        .def("as_int32", [](const freeusd::vt::Value& v) -> py::object {
          std::int32_t i{};
          if (v.GetInt32(&i)) {
            return py::cast(i);
          }
          return py::none();
        })
        .def("as_int64", [](const freeusd::vt::Value& v) -> py::object {
          std::int64_t i{};
          if (v.GetInt64(&i)) {
            return py::cast(i);
          }
          return py::none();
        })
        .def("as_float", [](const freeusd::vt::Value& v) -> py::object {
          float f{};
          if (v.GetFloat(&f)) {
            return py::cast(f);
          }
          return py::none();
        })
        .def("as_double", [](const freeusd::vt::Value& v) -> py::object {
          double d{};
          if (v.GetDouble(&d)) {
            return py::cast(d);
          }
          return py::none();
        })
        .def("as_string", [](const freeusd::vt::Value& v) -> py::object {
          std::string s;
          if (v.GetString(&s)) {
            return py::cast(s);
          }
          return py::none();
        })
        .def("as_token", [](const freeusd::vt::Value& v) -> py::object {
          freeusd::tf::Token t;
          if (v.GetToken(&t)) {
            return py::cast(t);
          }
          return py::none();
        })
        .def("as_token_array", [](const freeusd::vt::Value& v) -> py::object {
          std::vector<freeusd::tf::Token> t;
          if (v.GetTokenArray(&t)) {
            py::list lst;
            for (const freeusd::tf::Token& x : t) {
              lst.append(py::cast(x.GetText()));
            }
            return lst;
          }
          return py::none();
        })
        .def("as_quatd", [](const freeusd::vt::Value& v) -> py::object {
          freeusd::gf::Quatd q;
          if (v.GetQuatd(&q)) {
            return py::cast(q);
          }
          return py::none();
        })
        .def("as_quatf", [](const freeusd::vt::Value& v) -> py::object {
          freeusd::gf::Quatf q;
          if (v.GetQuatf(&q)) {
            return py::cast(q);
          }
          return py::none();
        })
        .def("as_matrix4d", [](const freeusd::vt::Value& v) -> py::object {
          freeusd::gf::Matrix4d m;
          if (v.GetMatrix4d(&m)) {
            return py::cast(m);
          }
          return py::none();
        })
        .def("as_vec3d", [](const freeusd::vt::Value& v) -> py::object {
          freeusd::gf::Vec3d out;
          if (v.GetVec3d(&out)) {
            return py::cast(out);
          }
          return py::none();
        })
        .def("as_vec3f", [](const freeusd::vt::Value& v) -> py::object {
          freeusd::gf::Vec3f out;
          if (v.GetVec3f(&out)) {
            return py::cast(out);
          }
          return py::none();
        })
        .def("__repr__", [](const freeusd::vt::Value& v) {
          std::ostringstream oss;
          oss << v;
          return oss.str();
        });
  }

  {
    auto sdf = m.def_submodule("sdf");
    py::class_<freeusd::sdf::Path>(sdf, "Path")
        .def(py::init<>())
        .def_static("from_string", [](std::string s) { return freeusd::sdf::Path::FromString(s); })
        .def_static("absolute_root", &freeusd::sdf::Path::AbsoluteRootPath)
        .def("is_empty", &freeusd::sdf::Path::IsEmpty)
        .def("is_absolute", &freeusd::sdf::Path::IsAbsolutePath)
        .def("is_absolute_root", &freeusd::sdf::Path::IsAbsoluteRootPath)
        .def("is_prim_path", &freeusd::sdf::Path::IsPrimPath)
        .def("is_property_path", &freeusd::sdf::Path::IsPropertyPath)
        .def("text", &freeusd::sdf::Path::GetText)
        .def("name", &freeusd::sdf::Path::GetName)
        .def("parent", &freeusd::sdf::Path::GetParentPath)
        .def("prim_path", &freeusd::sdf::Path::GetPrimPath)
        .def("element_count", &freeusd::sdf::Path::GetPathElementCount)
        .def("has_prefix", &freeusd::sdf::Path::HasPrefix)
        .def("variant_selections", [](const freeusd::sdf::Path& p) {
          py::list out;
          for (const auto& sel : p.GetVariantSelections()) {
            out.append(py::make_tuple(sel.variant_set, sel.variant_name));
          }
          return out;
        })
        .def("append_child", [](const freeusd::sdf::Path& p, const freeusd::tf::Token& n) { return p.AppendChild(n); })
        .def("__eq__", [](const freeusd::sdf::Path& a, const freeusd::sdf::Path& b) { return a == b; })
        .def("__repr__", [](const freeusd::sdf::Path& p) { return "<freeusd.sdf.Path '" + p.GetText() + "'>"; });

    py::class_<freeusd::sdf::PrimReference>(sdf, "PrimReference")
        .def(py::init<>())
        .def(py::init<std::string, std::optional<freeusd::sdf::Path>>(), py::arg("asset_path"),
             py::arg("prim_path") = std::nullopt)
        .def_readwrite("asset_path", &freeusd::sdf::PrimReference::asset_path)
        .def_readwrite("prim_path", &freeusd::sdf::PrimReference::prim_path)
        .def("is_empty", &freeusd::sdf::PrimReference::IsEmpty)
        .def("format_authored_for_usda", &freeusd::sdf::PrimReference::FormatAuthoredForUsda)
        .def_static("parse_authored", [](std::string text) -> py::object {
          freeusd::sdf::PrimReference r;
          if (!freeusd::sdf::PrimReference::ParseAuthored(text, &r)) {
            return py::none();
          }
          return py::cast(r);
        });

    py::class_<freeusd::sdf::AssetPath>(sdf, "AssetPath")
        .def(py::init<>())
        .def(py::init<std::string>(), py::arg("path"))
        .def("is_empty", &freeusd::sdf::AssetPath::IsEmpty)
        .def("path", &freeusd::sdf::AssetPath::GetPath)
        .def(
            "set_path",
            [](freeusd::sdf::AssetPath& a, std::string p) { a.SetPath(std::move(p)); },
            py::arg("path"))
        .def("__eq__", [](const freeusd::sdf::AssetPath& a, const freeusd::sdf::AssetPath& b) { return a == b; })
        .def("__repr__", [](const freeusd::sdf::AssetPath& a) {
          return std::string{"<freeusd.sdf.AssetPath '"} + a.GetPath() + "'>";
        });

    py::class_<freeusd::sdf::LayerOffset>(sdf, "LayerOffset")
        .def(py::init<>())
        .def(py::init<double, double>(), py::arg("offset") = 0.0, py::arg("scale") = 1.0)
        .def_readwrite("offset", &freeusd::sdf::LayerOffset::offset)
        .def_readwrite("scale", &freeusd::sdf::LayerOffset::scale)
        .def("is_identity", &freeusd::sdf::LayerOffset::IsIdentity);

    py::enum_<freeusd::sdf::Layer::PrimSpecifierKind>(sdf, "PrimSpecifierKind")
        .value("default_", freeusd::sdf::Layer::PrimSpecifierKind::Default)
        .value("def_", freeusd::sdf::Layer::PrimSpecifierKind::Def)
        .value("class_", freeusd::sdf::Layer::PrimSpecifierKind::Class)
        .value("over", freeusd::sdf::Layer::PrimSpecifierKind::Over)
        .export_values();

    py::class_<freeusd::sdf::Layer, std::shared_ptr<freeusd::sdf::Layer>>(sdf, "Layer")
        .def_static("new_anonymous", &freeusd::sdf::Layer::NewAnonymous, py::arg("identifier") = std::string{})
        .def("documentation", [](const freeusd::sdf::Layer& layer) { return layer.GetDocumentation(); })
        .def(
            "set_documentation",
            [](freeusd::sdf::Layer& layer, std::string d) { layer.SetDocumentation(std::move(d)); })
        .def("comment", [](const freeusd::sdf::Layer& layer) { return layer.GetComment(); })
        .def("set_comment", [](freeusd::sdf::Layer& layer, std::string c) { layer.SetComment(std::move(c)); })
        .def("clear_comment", &freeusd::sdf::Layer::ClearComment)
        .def("clear_sub_layer_offsets", &freeusd::sdf::Layer::ClearSubLayerOffsets)
        .def(
            "set_sub_layer_offset",
            [](freeusd::sdf::Layer& layer, std::string path, const freeusd::sdf::LayerOffset& off) {
              layer.SetSubLayerOffset(std::move(path), off);
            })
        .def("erase_sub_layer_offset", &freeusd::sdf::Layer::EraseSubLayerOffset)
        .def(
            "has_sub_layer_offset",
            [](const freeusd::sdf::Layer& layer, const std::string& path) { return layer.HasSubLayerOffset(path); })
        .def(
            "get_sub_layer_offset",
            [](const freeusd::sdf::Layer& layer, const std::string& path) -> py::object {
              freeusd::sdf::LayerOffset off{};
              if (layer.GetSubLayerOffset(path, &off)) {
                return py::cast(off);
              }
              return py::none();
            })
        .def(
            "list_sub_layer_offsets",
            [](const freeusd::sdf::Layer& layer) { return layer.ListSubLayerOffsets(); })
        .def(
            "default_prim",
            [](const freeusd::sdf::Layer& layer) -> py::object {
              const auto d = layer.GetDefaultPrim();
              return d.has_value() ? py::cast(std::string(*d)) : py::none();
            })
        .def("clear_default_prim", &freeusd::sdf::Layer::ClearDefaultPrim)
        .def(
            "set_default_prim",
            [](freeusd::sdf::Layer& layer, std::string n) {
              layer.SetDefaultPrim(n);
            })
        .def(
            "start_time_code",
            [](const freeusd::sdf::Layer& layer) -> py::object {
              const auto v = layer.GetStartTimeCode();
              return v.has_value() ? py::cast(*v) : py::none();
            })
        .def("set_start_time_code", [](freeusd::sdf::Layer& layer, double v) { layer.SetStartTimeCode(v); })
        .def("clear_start_time_code", &freeusd::sdf::Layer::ClearStartTimeCode)
        .def(
            "end_time_code",
            [](const freeusd::sdf::Layer& layer) -> py::object {
              const auto v = layer.GetEndTimeCode();
              return v.has_value() ? py::cast(*v) : py::none();
            })
        .def("set_end_time_code", [](freeusd::sdf::Layer& layer, double v) { layer.SetEndTimeCode(v); })
        .def("clear_end_time_code", &freeusd::sdf::Layer::ClearEndTimeCode)
        .def(
            "time_codes_per_second",
            [](const freeusd::sdf::Layer& layer) -> py::object {
              const auto v = layer.GetTimeCodesPerSecond();
              return v.has_value() ? py::cast(*v) : py::none();
            })
        .def("set_time_codes_per_second", [](freeusd::sdf::Layer& layer, double v) { layer.SetTimeCodesPerSecond(v); })
        .def("clear_time_codes_per_second", &freeusd::sdf::Layer::ClearTimeCodesPerSecond)
        .def(
            "frames_per_second",
            [](const freeusd::sdf::Layer& layer) -> py::object {
              const auto v = layer.GetFramesPerSecond();
              return v.has_value() ? py::cast(*v) : py::none();
            })
        .def("set_frames_per_second", [](freeusd::sdf::Layer& layer, double v) { layer.SetFramesPerSecond(v); })
        .def("clear_frames_per_second", &freeusd::sdf::Layer::ClearFramesPerSecond)
        .def(
            "frame_precision",
            [](const freeusd::sdf::Layer& layer) -> py::object {
              const auto v = layer.GetFramePrecision();
              return v.has_value() ? py::cast(*v) : py::none();
            })
        .def("set_frame_precision", [](freeusd::sdf::Layer& layer, int v) { layer.SetFramePrecision(v); })
        .def("clear_frame_precision", &freeusd::sdf::Layer::ClearFramePrecision)
        .def(
            "meters_per_unit",
            [](const freeusd::sdf::Layer& layer) -> py::object {
              const auto v = layer.GetMetersPerUnit();
              return v.has_value() ? py::cast(*v) : py::none();
            })
        .def("set_meters_per_unit", [](freeusd::sdf::Layer& layer, double v) { layer.SetMetersPerUnit(v); })
        .def("clear_meters_per_unit", &freeusd::sdf::Layer::ClearMetersPerUnit)
        .def(
            "up_axis",
            [](const freeusd::sdf::Layer& layer) -> py::object {
              const auto v = layer.GetUpAxis();
              return v.has_value() ? py::cast(*v) : py::none();
            })
        .def("set_up_axis", [](freeusd::sdf::Layer& layer, std::string a) { layer.SetUpAxis(std::move(a)); })
        .def("clear_up_axis", &freeusd::sdf::Layer::ClearUpAxis)
        .def("prim_order", [](const freeusd::sdf::Layer& layer) { return layer.GetPrimOrder(); })
        .def(
            "set_prim_order",
            [](freeusd::sdf::Layer& layer, std::vector<freeusd::sdf::Path> paths) {
              layer.SetPrimOrder(std::move(paths));
            })
        .def("clear_prim_order", &freeusd::sdf::Layer::ClearPrimOrder)
        .def(
            "set_sub_layers",
            [](freeusd::sdf::Layer& layer, std::vector<std::string> p) {
              layer.SetSubLayers(std::move(p));
            })
        .def("clear_sub_layers", &freeusd::sdf::Layer::ClearSubLayers)
        .def("has_default_prim", &freeusd::sdf::Layer::HasDefaultPrim)
        .def(
            "sub_layers",
            [](const freeusd::sdf::Layer& layer) { return layer.GetSubLayers(); })
        .def("clear_relocates", &freeusd::sdf::Layer::ClearRelocates)
        .def("set_relocate", &freeusd::sdf::Layer::SetRelocate)
        .def("erase_relocate", &freeusd::sdf::Layer::EraseRelocate)
        .def(
            "has_relocate",
            [](const freeusd::sdf::Layer& layer, const freeusd::sdf::Path& from) {
              return layer.HasRelocate(from);
            })
        .def(
            "get_relocate_target",
            [](const freeusd::sdf::Layer& layer, const freeusd::sdf::Path& from) -> py::object {
              freeusd::sdf::Path to;
              if (layer.GetRelocateTarget(from, &to)) {
                return py::cast(to);
              }
              return py::none();
            })
        .def(
            "list_relocates",
            [](const freeusd::sdf::Layer& layer) { return layer.ListRelocates(); })
        .def("clear_prefix_substitutions", &freeusd::sdf::Layer::ClearPrefixSubstitutions)
        .def(
            "set_prefix_substitution",
            [](freeusd::sdf::Layer& layer, std::string from_p, std::string to_p) {
              layer.SetPrefixSubstitution(std::move(from_p), std::move(to_p));
            })
        .def("erase_prefix_substitution", &freeusd::sdf::Layer::ErasePrefixSubstitution)
        .def(
            "has_prefix_substitution",
            [](const freeusd::sdf::Layer& layer, const std::string& from_p) {
              return layer.HasPrefixSubstitution(from_p);
            })
        .def(
            "get_prefix_substitution",
            [](const freeusd::sdf::Layer& layer, const std::string& from_p) -> py::object {
              std::string to;
              if (layer.GetPrefixSubstitution(from_p, &to)) {
                return py::cast(to);
              }
              return py::none();
            })
        .def(
            "list_prefix_substitutions",
            [](const freeusd::sdf::Layer& layer) { return layer.ListPrefixSubstitutions(); })
        .def("clear_custom_layer_data", &freeusd::sdf::Layer::ClearCustomLayerData)
        .def(
            "set_custom_layer_data_entry",
            [](freeusd::sdf::Layer& layer, std::string key, const freeusd::vt::Value& value) {
              layer.SetCustomLayerDataEntry(std::move(key), value);
            })
        .def("erase_custom_layer_data_entry", &freeusd::sdf::Layer::EraseCustomLayerDataEntry)
        .def(
            "has_custom_layer_data_key",
            [](const freeusd::sdf::Layer& layer, const std::string& key) {
              return layer.HasCustomLayerDataKey(key);
            })
        .def(
            "get_custom_layer_data_entry",
            [](const freeusd::sdf::Layer& layer, const std::string& key) -> py::object {
              freeusd::vt::Value v;
              if (layer.GetCustomLayerDataEntry(key, &v)) {
                return py::cast(v);
              }
              return py::none();
            })
        .def(
            "list_custom_layer_data_keys",
            [](const freeusd::sdf::Layer& layer) { return layer.ListCustomLayerDataKeys(); })
        .def("identifier", &freeusd::sdf::Layer::GetIdentifier, py::return_value_policy::copy)
        .def("set_field", &freeusd::sdf::Layer::SetField)
        .def("has_field", &freeusd::sdf::Layer::HasField)
        .def("get_field", [](const freeusd::sdf::Layer& layer, const freeusd::sdf::Path& p, const freeusd::tf::Token& t) -> py::object {
          freeusd::vt::Value v;
          if (layer.GetField(p, t, &v)) {
            return py::cast(v);
          }
          return py::none();
        })
        .def("list_prim_paths", &freeusd::sdf::Layer::ListPrimPaths)
        .def("list_field_names", &freeusd::sdf::Layer::ListFieldNames)
        .def(
            "set_relationship_targets",
            [](freeusd::sdf::Layer& layer, const freeusd::sdf::Path& p, const freeusd::tf::Token& rel,
               std::vector<freeusd::sdf::Path> targets) {
              layer.SetRelationshipTargets(p, rel, std::move(targets));
            })
        .def(
            "prepend_relationship_targets",
            [](freeusd::sdf::Layer& layer, const freeusd::sdf::Path& p, const freeusd::tf::Token& rel,
               std::vector<freeusd::sdf::Path> extra) {
              layer.PrependRelationshipTargets(p, rel, std::move(extra));
            })
        .def(
            "append_relationship_targets",
            [](freeusd::sdf::Layer& layer, const freeusd::sdf::Path& p, const freeusd::tf::Token& rel,
               std::vector<freeusd::sdf::Path> extra) {
              layer.AppendRelationshipTargets(p, rel, std::move(extra));
            })
        .def(
            "delete_relationship_targets",
            [](freeusd::sdf::Layer& layer, const freeusd::sdf::Path& p, const freeusd::tf::Token& rel,
               std::vector<freeusd::sdf::Path> remove_targets) {
              layer.DeleteRelationshipTargets(p, rel, std::move(remove_targets));
            })
        .def("clear_relationship", &freeusd::sdf::Layer::ClearRelationship)
        .def("has_relationship", &freeusd::sdf::Layer::HasRelationship)
        .def("get_relationship_targets", &freeusd::sdf::Layer::GetRelationshipTargets)
        .def("list_relationship_names", &freeusd::sdf::Layer::ListRelationshipNames)
        .def("clear", &freeusd::sdf::Layer::Clear)
        .def("set_prim_kind", &freeusd::sdf::Layer::SetPrimKind)
        .def("has_prim_kind", &freeusd::sdf::Layer::HasPrimKind)
        .def("get_prim_kind", &freeusd::sdf::Layer::GetPrimKind)
        .def("clear_prim_custom_data", &freeusd::sdf::Layer::ClearPrimCustomData)
        .def(
            "set_prim_custom_data_entry",
            [](freeusd::sdf::Layer& layer, const freeusd::sdf::Path& prim_path, std::string key,
               const freeusd::vt::Value& value) { layer.SetPrimCustomDataEntry(prim_path, std::move(key), value); })
        .def(
            "erase_prim_custom_data_entry",
            [](freeusd::sdf::Layer& layer, const freeusd::sdf::Path& prim_path, const std::string& key) {
              layer.ErasePrimCustomDataEntry(prim_path, key);
            })
        .def(
            "has_prim_custom_data_key",
            [](const freeusd::sdf::Layer& layer, const freeusd::sdf::Path& prim_path, const std::string& key) {
              return layer.HasPrimCustomDataKey(prim_path, key);
            })
        .def(
            "get_prim_custom_data_entry",
            [](const freeusd::sdf::Layer& layer, const freeusd::sdf::Path& prim_path,
               const std::string& key) -> py::object {
              freeusd::vt::Value v;
              if (layer.GetPrimCustomDataEntry(prim_path, key, &v)) {
                return py::cast(v);
              }
              return py::none();
            })
        .def(
            "list_prim_custom_data_keys",
            [](const freeusd::sdf::Layer& layer, const freeusd::sdf::Path& prim_path) {
              return layer.ListPrimCustomDataKeys(prim_path);
            })
        .def("clear_prim_variant_selection", &freeusd::sdf::Layer::ClearPrimVariantSelection)
        .def(
            "set_prim_variant_selection_entry",
            [](freeusd::sdf::Layer& layer, const freeusd::sdf::Path& prim_path, std::string variant_set,
               std::string variant_name) {
              layer.SetPrimVariantSelectionEntry(prim_path, std::move(variant_set), std::move(variant_name));
            })
        .def(
            "erase_prim_variant_selection_entry",
            [](freeusd::sdf::Layer& layer, const freeusd::sdf::Path& prim_path, const std::string& variant_set) {
              layer.ErasePrimVariantSelectionEntry(prim_path, variant_set);
            })
        .def(
            "has_prim_variant_selection_key",
            [](const freeusd::sdf::Layer& layer, const freeusd::sdf::Path& prim_path, const std::string& variant_set) {
              return layer.HasPrimVariantSelectionKey(prim_path, variant_set);
            })
        .def(
            "get_prim_variant_selection_entry",
            [](const freeusd::sdf::Layer& layer, const freeusd::sdf::Path& prim_path,
               const std::string& variant_set) -> py::object {
              std::string name;
              if (layer.GetPrimVariantSelectionEntry(prim_path, variant_set, &name)) {
                return py::str(name);
              }
              return py::none();
            })
        .def(
            "list_prim_variant_selection_sets",
            [](const freeusd::sdf::Layer& layer, const freeusd::sdf::Path& prim_path) {
              return layer.ListPrimVariantSelectionSets(prim_path);
            })
        .def("clear_prim_variant_sets", &freeusd::sdf::Layer::ClearPrimVariantSets)
        .def(
            "set_prim_variant_set_variants",
            [](freeusd::sdf::Layer& layer, const freeusd::sdf::Path& prim_path, std::string set_name,
               std::vector<std::string> names) {
              layer.SetPrimVariantSetVariants(prim_path, std::move(set_name), std::move(names));
            })
        .def(
            "has_prim_variant_set",
            [](const freeusd::sdf::Layer& layer, const freeusd::sdf::Path& prim_path, const std::string& set_name) {
              return layer.HasPrimVariantSet(prim_path, set_name);
            })
        .def(
            "list_prim_variant_set_names",
            [](const freeusd::sdf::Layer& layer, const freeusd::sdf::Path& prim_path) {
              return layer.ListPrimVariantSetNames(prim_path);
            })
        .def(
            "list_prim_variant_names",
            [](const freeusd::sdf::Layer& layer, const freeusd::sdf::Path& prim_path, const std::string& set_name) {
              return layer.ListPrimVariantNames(prim_path, set_name);
            })
        .def(
            "set_attribute_connection",
            [](freeusd::sdf::Layer& layer, const freeusd::sdf::Path& prim_path, const freeusd::tf::Token& name,
               const freeusd::sdf::Path& target_prop) {
              layer.SetAttributeConnection(prim_path, name, target_prop);
            })
        .def(
            "clear_attribute_connection",
            [](freeusd::sdf::Layer& layer, const freeusd::sdf::Path& prim_path, const freeusd::tf::Token& name) {
              layer.ClearAttributeConnection(prim_path, name);
            })
        .def(
            "has_attribute_connection",
            [](const freeusd::sdf::Layer& layer, const freeusd::sdf::Path& prim_path, const freeusd::tf::Token& name) {
              return layer.HasAttributeConnection(prim_path, name);
            })
        .def(
            "get_attribute_connection_target",
            [](const freeusd::sdf::Layer& layer, const freeusd::sdf::Path& prim_path,
               const freeusd::tf::Token& name) -> py::object {
              freeusd::sdf::Path t;
              if (layer.GetAttributeConnectionTarget(prim_path, name, &t)) {
                return py::cast(t);
              }
              return py::none();
            })
        .def(
            "list_attribute_connections",
            [](const freeusd::sdf::Layer& layer, const freeusd::sdf::Path& prim_path) {
              return layer.ListAttributeConnections(prim_path);
            })
        .def(
            "add_reference",
            [](freeusd::sdf::Layer& layer, const freeusd::sdf::Path& p, std::string a) {
              layer.AddReference(p, std::move(a));
            })
        .def(
            "set_references",
            [](freeusd::sdf::Layer& layer, const freeusd::sdf::Path& p, std::vector<std::string> assets) {
              layer.SetReferences(p, std::move(assets));
            })
        .def("clear_references", &freeusd::sdf::Layer::ClearReferences)
        .def(
            "list_references",
            [](const freeusd::sdf::Layer& layer, const freeusd::sdf::Path& p) { return layer.ListReferences(p); })
        .def("add_prim_reference", &freeusd::sdf::Layer::AddPrimReference)
        .def(
            "set_prim_references",
            [](freeusd::sdf::Layer& layer, const freeusd::sdf::Path& p,
               std::vector<freeusd::sdf::PrimReference> refs) {
              layer.SetPrimReferences(p, std::move(refs));
            })
        .def(
            "list_prim_references",
            [](const freeusd::sdf::Layer& layer, const freeusd::sdf::Path& p) {
              return layer.ListPrimReferences(p);
            })
        .def("clear_prim_inherits", &freeusd::sdf::Layer::ClearPrimInherits)
        .def("add_prim_inherit", &freeusd::sdf::Layer::AddPrimInherit)
        .def(
            "prepend_prim_inherits",
            [](freeusd::sdf::Layer& layer, const freeusd::sdf::Path& p, std::vector<freeusd::sdf::Path> paths) {
              layer.PrependPrimInherits(p, std::move(paths));
            })
        .def(
            "append_prim_inherits",
            [](freeusd::sdf::Layer& layer, const freeusd::sdf::Path& p, std::vector<freeusd::sdf::Path> paths) {
              layer.AppendPrimInherits(p, std::move(paths));
            })
        .def(
            "set_prim_inherits",
            [](freeusd::sdf::Layer& layer, const freeusd::sdf::Path& p, std::vector<freeusd::sdf::Path> paths) {
              layer.SetPrimInherits(p, std::move(paths));
            })
        .def("list_prim_inherits", &freeusd::sdf::Layer::ListPrimInherits)
        .def("clear_prim_specializes", &freeusd::sdf::Layer::ClearPrimSpecializes)
        .def("add_prim_specializes", &freeusd::sdf::Layer::AddPrimSpecializes)
        .def(
            "prepend_prim_specializes",
            [](freeusd::sdf::Layer& layer, const freeusd::sdf::Path& p, std::vector<freeusd::sdf::Path> paths) {
              layer.PrependPrimSpecializes(p, std::move(paths));
            })
        .def(
            "append_prim_specializes",
            [](freeusd::sdf::Layer& layer, const freeusd::sdf::Path& p, std::vector<freeusd::sdf::Path> paths) {
              layer.AppendPrimSpecializes(p, std::move(paths));
            })
        .def(
            "set_prim_specializes",
            [](freeusd::sdf::Layer& layer, const freeusd::sdf::Path& p, std::vector<freeusd::sdf::Path> paths) {
              layer.SetPrimSpecializes(p, std::move(paths));
            })
        .def("list_prim_specializes", &freeusd::sdf::Layer::ListPrimSpecializes)
        .def("clear_prim_payloads", &freeusd::sdf::Layer::ClearPrimPayloads)
        .def("add_prim_payload", &freeusd::sdf::Layer::AddPrimPayload)
        .def(
            "set_prim_payloads",
            [](freeusd::sdf::Layer& layer, const freeusd::sdf::Path& p,
               std::vector<freeusd::sdf::PrimReference> refs) {
              layer.SetPrimPayloads(p, std::move(refs));
            })
        .def("list_prim_payloads", &freeusd::sdf::Layer::ListPrimPayloads)
        .def("list_payloads", [](const freeusd::sdf::Layer& layer, const freeusd::sdf::Path& p) {
          return layer.ListPayloads(p);
        })
        .def(
            "is_prim_active",
            [](const freeusd::sdf::Layer& layer, const freeusd::sdf::Path& p) {
              return layer.IsPrimActive(p);
            })
        .def(
            "set_prim_active",
            [](freeusd::sdf::Layer& layer, const freeusd::sdf::Path& p, bool active) {
              layer.SetPrimActive(p, active);
            })
        .def("has_prim_active_opinion", &freeusd::sdf::Layer::HasPrimActiveOpinion)
        .def(
            "get_prim_specifier",
            [](const freeusd::sdf::Layer& layer, const freeusd::sdf::Path& p) {
              return layer.GetPrimSpecifier(p);
            })
        .def("has_prim_specifier_opinion", &freeusd::sdf::Layer::HasPrimSpecifierOpinion)
        .def(
            "set_prim_specifier",
            [](freeusd::sdf::Layer& layer, const freeusd::sdf::Path& p,
               freeusd::sdf::Layer::PrimSpecifierKind sk) {
              layer.SetPrimSpecifier(p, sk);
            })
        .def(
            "get_field_at_time",
            [](const freeusd::sdf::Layer& layer, const freeusd::sdf::Path& p, const freeusd::tf::Token& t, double time) -> py::object {
              freeusd::vt::Value v;
              if (layer.GetFieldAtTime(p, t, time, &v)) {
                return py::cast(v);
              }
              return py::none();
            })
        .def("set_time_sample", &freeusd::sdf::Layer::SetTimeSample)
        .def("list_sample_times", &freeusd::sdf::Layer::ListSampleTimes)
        .def(
            "get_time_sample",
            [](const freeusd::sdf::Layer& layer, const freeusd::sdf::Path& p, const freeusd::tf::Token& t, double time) -> py::object {
              freeusd::vt::Value v;
              if (layer.GetTimeSample(p, t, time, &v)) {
                return py::cast(v);
              }
              return py::none();
            })
        .def("clear_time_samples", &freeusd::sdf::Layer::ClearTimeSamples);

    {
      auto bt = sdf.def_submodule("builtin_tokens");
      bt.def("SubLayers", [] { return freeusd::sdf::tokens::SubLayers(); });
      bt.def("SubLayerOffsets", [] { return freeusd::sdf::tokens::SubLayerOffsets(); });
      bt.def("DefaultPrim", [] { return freeusd::sdf::tokens::DefaultPrim(); });
      bt.def("Documentation", [] { return freeusd::sdf::tokens::Documentation(); });
      bt.def("Comment", [] { return freeusd::sdf::tokens::Comment(); });
      bt.def("PrimOrder", [] { return freeusd::sdf::tokens::PrimOrder(); });
      bt.def("MetersPerUnit", [] { return freeusd::sdf::tokens::MetersPerUnit(); });
      bt.def("UpAxis", [] { return freeusd::sdf::tokens::UpAxis(); });
      bt.def("StartTimeCode", [] { return freeusd::sdf::tokens::StartTimeCode(); });
      bt.def("EndTimeCode", [] { return freeusd::sdf::tokens::EndTimeCode(); });
      bt.def("TimeCodesPerSecond", [] { return freeusd::sdf::tokens::TimeCodesPerSecond(); });
      bt.def("FramesPerSecond", [] { return freeusd::sdf::tokens::FramesPerSecond(); });
      bt.def("FramePrecision", [] { return freeusd::sdf::tokens::FramePrecision(); });
      bt.def("Kind", [] { return freeusd::sdf::tokens::Kind(); });
      bt.def("Active", [] { return freeusd::sdf::tokens::Active(); });
      bt.def("CustomData", [] { return freeusd::sdf::tokens::CustomData(); });
      bt.def("CustomLayerData", [] { return freeusd::sdf::tokens::CustomLayerData(); });
      bt.def("VariantSetNames", [] { return freeusd::sdf::tokens::VariantSetNames(); });
    }
  }

  {
    auto io = m.def_submodule("io");
    py::class_<freeusd::io::usda::ParseResult>(io, "ParseResult")
        .def_readonly("ok", &freeusd::io::usda::ParseResult::ok)
        .def_readonly("line", &freeusd::io::usda::ParseResult::line)
        .def_readonly("message", &freeusd::io::usda::ParseResult::message);

    io.def("load_from_string", [](std::string text, std::shared_ptr<freeusd::sdf::Layer> layer) {
      return freeusd::io::usda::LoadFromString(text, layer);
    });
    io.def("save_to_string", [](const freeusd::sdf::Layer& layer) { return freeusd::io::usda::SaveToString(layer); });
    io.def("load_from_file", [](const std::string& path, std::shared_ptr<freeusd::sdf::Layer> layer) {
      return freeusd::io::usda::LoadFromFile(path, layer);
    });
    io.def("save_to_file", [](const std::string& path, const freeusd::sdf::Layer& layer) {
      return freeusd::io::usda::SaveToFile(path, layer);
    });
  }

  {
    auto ar = m.def_submodule("ar");
    py::class_<freeusd::ar::ResolvedPath>(ar, "ResolvedPath")
        .def(py::init<>())
        .def(py::init<std::string>(), py::arg("path"))
        .def("is_empty", &freeusd::ar::ResolvedPath::IsEmpty)
        .def("path", &freeusd::ar::ResolvedPath::GetPath)
        .def(
            "set_path",
            [](freeusd::ar::ResolvedPath& rp, std::string p) { rp.SetPath(std::move(p)); },
            py::arg("path"))
        .def("__eq__", [](const freeusd::ar::ResolvedPath& a, const freeusd::ar::ResolvedPath& b) { return a == b; })
        .def("__repr__", [](const freeusd::ar::ResolvedPath& r) {
          return std::string{"<freeusd.ar.ResolvedPath '"} + r.GetPath() + "'>";
        });

    py::class_<freeusd::ar::DefaultResolver>(ar, "DefaultResolver")
        .def(py::init<std::string>(), py::arg("anchor_directory") = std::string{})
        .def("resolve", [](freeusd::ar::DefaultResolver& r, const std::string& p) { return r.Resolve(p); })
        .def(
            "resolve_path",
            [](freeusd::ar::DefaultResolver& r, const std::string& p) {
              return freeusd::ar::ResolvedPath{r.Resolve(p)};
            },
            py::arg("asset_path"));
  }

  {
    auto pcp = m.def_submodule("pcp");
    py::class_<freeusd::pcp::LayerStack>(pcp, "LayerStack")
        .def(py::init<>())
        .def("append", &freeusd::pcp::LayerStack::Append)
        .def("clear", &freeusd::pcp::LayerStack::Clear)
        .def("is_empty", &freeusd::pcp::LayerStack::IsEmpty)
        .def(
            "layers",
            [](const freeusd::pcp::LayerStack& s) { return s.GetLayers(); },
            py::return_value_policy::reference_internal);

    pcp.def(
        "compose_sublayers",
        [](const std::shared_ptr<freeusd::sdf::Layer>& root,
           const std::function<std::shared_ptr<freeusd::sdf::Layer>(const std::string&)>& resolve) {
          return freeusd::pcp::ComposeSublayers(root, resolve);
        },
        py::arg("root"), py::arg("resolve"),
        R"pbdoc(
Compose root plus direct sublayers. ``resolve(authored_path)`` may return ``None`` to skip a path.
Order: root strongest, then authored ``subLayers`` entries (early entries stronger among sublayers).)pbdoc");

    pcp.def(
        "compose_sublayers_depth_first",
        [](const std::shared_ptr<freeusd::sdf::Layer>& root,
           const std::function<std::shared_ptr<freeusd::sdf::Layer>(const std::string&)>& resolve) {
          return freeusd::pcp::ComposeSublayersDepthFirst(root, resolve);
        },
        py::arg("root"), py::arg("resolve"),
        R"pbdoc(
Recursively compose sublayers depth-first under each resolved layer before the next sibling ordered
reference; breaks cycles encountered along the DFS stack.)pbdoc");
  }

  {
    auto usd = m.def_submodule("usd");
    {
      auto kind_tokens = usd.def_submodule("kind_tokens");
      kind_tokens.def("Component", [] { return freeusd::usd::kindTokens::Component(); });
      kind_tokens.def("Group", [] { return freeusd::usd::kindTokens::Group(); });
      kind_tokens.def("Assembly", [] { return freeusd::usd::kindTokens::Assembly(); });
      kind_tokens.def("Subcomponent", [] { return freeusd::usd::kindTokens::Subcomponent(); });
    }

    {
      auto ut = usd.def_submodule("builtin_tokens");
      ut.def("References", [] { return freeusd::usd::tokens::References(); });
      ut.def("Payload", [] { return freeusd::usd::tokens::Payload(); });
      ut.def("Inherits", [] { return freeusd::usd::tokens::Inherits(); });
      ut.def("Specializes", [] { return freeusd::usd::tokens::Specializes(); });
      ut.def("VariantSets", [] { return freeusd::usd::tokens::VariantSets(); });
      ut.def("VariantSelection", [] { return freeusd::usd::tokens::VariantSelection(); });
      ut.def("PrefixSubstitutions", [] { return freeusd::usd::tokens::PrefixSubstitutions(); });
      ut.def("Relocates", [] { return freeusd::usd::tokens::Relocates(); });
    }

    {
      auto usd_schema_data_tokens = usd.def_submodule("schema_data_tokens");
#include "generated/usd_schema_data_tokens.inc"
    }

    py::class_<freeusd::usd::TimeCode>(usd, "TimeCode")
        .def_static("Default", &freeusd::usd::TimeCode::Default)
        .def_static("EarliestTime", &freeusd::usd::TimeCode::EarliestTime)
        .def_static("FromDouble", &freeusd::usd::TimeCode::FromDouble, py::arg("t"))
        .def("is_default", &freeusd::usd::TimeCode::IsDefault)
        .def("is_earliest_time", &freeusd::usd::TimeCode::IsEarliestTime)
        .def("is_numeric", &freeusd::usd::TimeCode::IsNumeric)
        .def("value", &freeusd::usd::TimeCode::GetValue)
        .def("__eq__", [](const freeusd::usd::TimeCode& a, const freeusd::usd::TimeCode& b) { return a == b; })
        .def("__repr__", [](const freeusd::usd::TimeCode& t) {
          std::ostringstream oss;
          oss << t;
          return oss.str();
        });

    {
      auto crate = usd.def_submodule("crate");
      crate.doc() =
          "Binary crate: path sniffing, bootstrap header read, TOC section list read, and raw section payload access (little-endian; no spec-level decode yet).";
      crate.def("usdc_crate_identifier", [] { return std::string(freeusd::usd::crate::UsdcCrateIdentifier()); });
      py::enum_<freeusd::usd::crate::UsdFileKind>(crate, "UsdFileKind")
          .value("io_or_empty", freeusd::usd::crate::UsdFileKind::IoOrEmpty)
          .value("usda_ascii", freeusd::usd::crate::UsdFileKind::UsdaAscii)
          .value("usdc_crate", freeusd::usd::crate::UsdFileKind::UsdcCrate)
          .value("unknown", freeusd::usd::crate::UsdFileKind::Unknown);
      crate.def(
          "detect_usd_file_kind_from_path",
          [](const std::string& path) {
            std::string detail;
            const auto k = freeusd::usd::crate::DetectUsdFileKindFromPath(path, &detail);
            return py::make_tuple(k, detail);
          },
          py::arg("path"));
      crate.def(
          "read_usdc_bootstrap_from_path",
          [](const std::string& path) {
            freeusd::usd::crate::UsdcCrateBootstrap b{};
            std::string err;
            if (!freeusd::usd::crate::ReadUsdCrateBootstrapFromPath(path, b, &err)) {
              return py::make_tuple(false, py::none(), err);
            }
            py::dict d;
            d["file_version_major"] = static_cast<int>(b.file_version_major);
            d["file_version_minor"] = static_cast<int>(b.file_version_minor);
            d["file_version_patch"] = static_cast<int>(b.file_version_patch);
            d["toc_byte_offset"] = b.toc_byte_offset;
            return py::make_tuple(true, d, std::string{});
          },
          py::arg("path"));
      crate.def(
          "read_usdc_toc_from_path",
          [](const std::string& path, std::size_t max_sections) {
            freeusd::usd::crate::UsdcCrateToc toc{};
            std::string err;
            if (!freeusd::usd::crate::ReadUsdCrateTocFromPath(path, toc, max_sections, &err)) {
              return py::make_tuple(false, py::none(), err);
            }
            py::dict out;
            out["section_count"] = toc.section_count;
            py::list secs;
            for (const auto& s : toc.sections) {
              py::dict d;
              d["name"] = s.name;
              d["start_byte_offset"] = s.start_byte_offset;
              d["size_bytes"] = s.size_bytes;
              secs.append(d);
            }
            out["sections"] = secs;
            return py::make_tuple(true, out, std::string{});
          },
          py::arg("path"),
          py::arg("max_sections") = static_cast<std::size_t>(65536));
      crate.def(
          "read_usdc_section_bytes_from_path",
          [](const std::string& path, const std::string& section_name, std::size_t max_bytes) {
            std::vector<std::uint8_t> bytes;
            std::string err;
            if (!freeusd::usd::crate::ReadUsdCrateSectionBytesFromPath(path, section_name, bytes, max_bytes, &err)) {
              return py::make_tuple(false, py::none(), err);
            }
            return py::make_tuple(true, py::bytes(reinterpret_cast<const char*>(bytes.data()), bytes.size()),
                                  std::string{});
          },
          py::arg("path"),
          py::arg("section_name"),
          py::arg("max_bytes") = static_cast<std::size_t>(16u * 1024u * 1024u));
      crate.def(
          "read_usdc_token_table_from_path",
          [](const std::string& path, std::size_t max_entries, std::size_t max_total_bytes) {
            freeusd::usd::crate::UsdcCrateStringTable table{};
            std::string err;
            if (!freeusd::usd::crate::ReadUsdCrateTokenTableFromPath(path, table, max_entries, max_total_bytes, &err)) {
              return py::make_tuple(false, py::none(), err);
            }
            py::list items;
            for (const std::string& value : table.values) {
              items.append(value);
            }
            return py::make_tuple(true, items, std::string{});
          },
          py::arg("path"),
          py::arg("max_entries") = static_cast<std::size_t>(65536u),
          py::arg("max_total_bytes") = static_cast<std::size_t>(16u * 1024u * 1024u));
      crate.def(
          "read_usdc_string_table_from_path",
          [](const std::string& path, std::size_t max_entries, std::size_t max_total_bytes) {
            freeusd::usd::crate::UsdcCrateStringTable table{};
            std::string err;
            if (!freeusd::usd::crate::ReadUsdCrateStringTableFromPath(path, table, max_entries, max_total_bytes, &err)) {
              return py::make_tuple(false, py::none(), err);
            }
            py::list items;
            for (const std::string& value : table.values) {
              items.append(value);
            }
            return py::make_tuple(true, items, std::string{});
          },
          py::arg("path"),
          py::arg("max_entries") = static_cast<std::size_t>(65536u),
          py::arg("max_total_bytes") = static_cast<std::size_t>(16u * 1024u * 1024u));
      crate.def(
          "read_usdc_path_table_from_path",
          [](const std::string& path, std::size_t max_entries, std::size_t max_total_bytes) {
            freeusd::usd::crate::UsdcCratePathTable table{};
            std::string err;
            if (!freeusd::usd::crate::ReadUsdCratePathTableFromPath(path, table, max_entries, max_total_bytes, &err)) {
              return py::make_tuple(false, py::none(), err);
            }
            py::list items;
            for (const freeusd::sdf::Path& value : table.paths) {
              items.append(value);
            }
            return py::make_tuple(true, items, std::string{});
          },
          py::arg("path"),
          py::arg("max_entries") = static_cast<std::size_t>(65536u),
          py::arg("max_total_bytes") = static_cast<std::size_t>(16u * 1024u * 1024u));
    }

    py::class_<freeusd::usd::EditTarget>(usd, "EditTarget")
        .def(py::init<>())
        .def(py::init<std::shared_ptr<freeusd::sdf::Layer>>(), py::arg("layer"))
        .def_readwrite("layer", &freeusd::usd::EditTarget::layer)
        .def("is_valid", &freeusd::usd::EditTarget::IsValid);

    py::enum_<freeusd::usd::RootLayerSublayersPolicy>(usd, "RootLayerSublayersPolicy")
        .value("none", freeusd::usd::RootLayerSublayersPolicy::None)
        .value("shallow", freeusd::usd::RootLayerSublayersPolicy::Shallow)
        .value("depth_first", freeusd::usd::RootLayerSublayersPolicy::DepthFirst);

    py::class_<freeusd::usd::Stage, std::shared_ptr<freeusd::usd::Stage>>(usd, "Stage")
        .def_static("attach_root_layer", &freeusd::usd::Stage::AttachRootLayer)
        .def_static("attach_layer_stack", &freeusd::usd::Stage::AttachLayerStack)
        .def_static(
            "open_from_root_file",
            [](const std::string& path, freeusd::usd::RootLayerSublayersPolicy sublayers) {
              std::string err;
              auto st = freeusd::usd::Stage::OpenFromRootFile(path, sublayers, &err);
              if (!st) {
                throw py::value_error(err.empty() ? std::string{"open_from_root_file failed"} : err);
              }
              return st;
            },
            py::arg("path"),
            py::arg("sublayers") = freeusd::usd::RootLayerSublayersPolicy::DepthFirst)
        .def("pseudo_root_path", &freeusd::usd::Stage::GetPseudoRootPath)
        .def(
            "compose_layers",
            [](const freeusd::usd::Stage& st) { return st.GetComposeLayers(); })
        .def(
            "read_field_at_time",
            [](const freeusd::usd::Stage& st, const freeusd::sdf::Path& p, const freeusd::tf::Token& t,
               double time) -> py::object {
              return freeusd_py_composed_read::read_field_at_time(st, p, t, time);
            },
            py::arg("path"), py::arg("name"), py::arg("time") = 1.0)
        .def(
            "read_field_double",
            [](const freeusd::usd::Stage& st, const freeusd::sdf::Path& p, const freeusd::tf::Token& t,
               double time) -> py::object {
              return freeusd_py_composed_read::read_field_double(st, p, t, time);
            },
            py::arg("path"), py::arg("name"), py::arg("time") = 1.0)
        .def(
            "read_field_float",
            [](const freeusd::usd::Stage& st, const freeusd::sdf::Path& p, const freeusd::tf::Token& t,
               double time) -> py::object {
              return freeusd_py_composed_read::read_field_float(st, p, t, time);
            },
            py::arg("path"), py::arg("name"), py::arg("time") = 1.0)
        .def(
            "read_field_bool",
            [](const freeusd::usd::Stage& st, const freeusd::sdf::Path& p, const freeusd::tf::Token& t,
               double time) -> py::object {
              return freeusd_py_composed_read::read_field_bool(st, p, t, time);
            },
            py::arg("path"), py::arg("name"), py::arg("time") = 1.0)
        .def(
            "read_field_int64",
            [](const freeusd::usd::Stage& st, const freeusd::sdf::Path& p, const freeusd::tf::Token& t,
               double time) -> py::object {
              return freeusd_py_composed_read::read_field_int64(st, p, t, time);
            },
            py::arg("path"), py::arg("name"), py::arg("time") = 1.0)
        .def(
            "read_field_string",
            [](const freeusd::usd::Stage& st, const freeusd::sdf::Path& p, const freeusd::tf::Token& t,
               double time) -> py::object {
              return freeusd_py_composed_read::read_field_string(st, p, t, time);
            },
            py::arg("path"), py::arg("name"), py::arg("time") = 1.0)
        .def(
            "read_field_vec3d",
            [](const freeusd::usd::Stage& st, const freeusd::sdf::Path& p, const freeusd::tf::Token& t,
               double time) -> py::object {
              return freeusd_py_composed_read::read_field_vec3d(st, p, t, time);
            },
            py::arg("path"), py::arg("name"), py::arg("time") = 1.0)
        .def(
            "read_field_vec3f",
            [](const freeusd::usd::Stage& st, const freeusd::sdf::Path& p, const freeusd::tf::Token& t,
               double time) -> py::object {
              return freeusd_py_composed_read::read_field_vec3f(st, p, t, time);
            },
            py::arg("path"), py::arg("name"), py::arg("time") = 1.0)
        .def(
            "read_field_matrix4d",
            [](const freeusd::usd::Stage& st, const freeusd::sdf::Path& p, const freeusd::tf::Token& t,
               double time) -> py::object {
              return freeusd_py_composed_read::read_field_matrix4d(st, p, t, time);
            },
            py::arg("path"), py::arg("name"), py::arg("time") = 1.0)
        .def(
            "read_field_quatd",
            [](const freeusd::usd::Stage& st, const freeusd::sdf::Path& p, const freeusd::tf::Token& t,
               double time) -> py::object {
              return freeusd_py_composed_read::read_field_quatd(st, p, t, time);
            },
            py::arg("path"), py::arg("name"), py::arg("time") = 1.0)
        .def(
            "read_field_quatf",
            [](const freeusd::usd::Stage& st, const freeusd::sdf::Path& p, const freeusd::tf::Token& t,
               double time) -> py::object {
              return freeusd_py_composed_read::read_field_quatf(st, p, t, time);
            },
            py::arg("path"), py::arg("name"), py::arg("time") = 1.0)
        .def(
            "read_field_token",
            [](const freeusd::usd::Stage& st, const freeusd::sdf::Path& p, const freeusd::tf::Token& t,
               double time) -> py::object {
              return freeusd_py_composed_read::read_field_token(st, p, t, time);
            },
            py::arg("path"), py::arg("name"), py::arg("time") = 1.0)
        .def(
            "read_field_token_array",
            [](const freeusd::usd::Stage& st, const freeusd::sdf::Path& p, const freeusd::tf::Token& t,
               double time) -> py::object {
              return freeusd_py_composed_read::read_field_token_array(st, p, t, time);
            },
            py::arg("path"), py::arg("name"), py::arg("time") = 1.0)
        .def("has_field_opinion", &freeusd::usd::Stage::HasFieldOpinion)
        .def("has_attribute_connection", &freeusd::usd::Stage::HasAttributeConnection)
        .def(
            "get_composed_attribute_connection_target",
            [](const freeusd::usd::Stage& st, const freeusd::sdf::Path& p, const freeusd::tf::Token& attr) -> py::object {
              freeusd::sdf::Path tgt;
              if (st.GetComposedAttributeConnectionTarget(p, attr, &tgt)) {
                return py::cast(tgt);
              }
              return py::none();
            })
        .def(
            "read_relationship",
            [](const freeusd::usd::Stage& st, const freeusd::sdf::Path& p, const freeusd::tf::Token& rel) {
              std::vector<freeusd::sdf::Path> out;
              (void)st.ReadRelationship(p, rel, &out);
              return out;
            })
        .def("has_relationship", &freeusd::usd::Stage::HasRelationship)
        .def("read_prim_references", &freeusd::usd::Stage::ReadPrimReferences)
        .def("has_prim_references", &freeusd::usd::Stage::HasPrimReferences)
        .def("read_prim_inherits", &freeusd::usd::Stage::ReadPrimInherits)
        .def("has_prim_inherits", &freeusd::usd::Stage::HasPrimInherits)
        .def("read_prim_specializes", &freeusd::usd::Stage::ReadPrimSpecializes)
        .def("has_prim_specializes", &freeusd::usd::Stage::HasPrimSpecializes)
        .def("read_prim_payloads", &freeusd::usd::Stage::ReadPrimPayloads)
        .def("has_prim_payloads", &freeusd::usd::Stage::HasPrimPayloads)
        .def("resolve_prim_kind", &freeusd::usd::Stage::ResolvePrimKind)
        .def("resolve_has_prim_kind", &freeusd::usd::Stage::ResolveHasPrimKind)
        .def("resolve_prim_specifier_kind", &freeusd::usd::Stage::ResolvePrimSpecifierKind)
        .def("resolve_prim_active", &freeusd::usd::Stage::ResolvePrimActive)
        .def("resolve_has_prim_active_opinion", &freeusd::usd::Stage::ResolveHasPrimActiveOpinion)
        .def(
            "get_composed_prim_custom_data",
            [](const freeusd::usd::Stage& stage, const freeusd::sdf::Path& path, std::string key) -> py::object {
              freeusd::vt::Value v;
              if (stage.GetComposedPrimCustomData(path, key, &v)) {
                return py::cast(v);
              }
              return py::none();
            })
        .def(
            "prim_custom_data_key_in_any_layer",
            [](const freeusd::usd::Stage& stage, const freeusd::sdf::Path& path, const std::string& key) {
              return stage.PrimCustomDataKeyInAnyLayer(path, key);
            })
        .def(
            "list_composed_prim_custom_data_keys",
            [](const freeusd::usd::Stage& stage, const freeusd::sdf::Path& path) {
              return stage.ListComposedPrimCustomDataKeys(path);
            })
        .def(
            "get_composed_custom_layer_data",
            [](const freeusd::usd::Stage& stage, const std::string& key) -> py::object {
              freeusd::vt::Value v;
              if (stage.GetComposedCustomLayerData(key, &v)) {
                return py::cast(v);
              }
              return py::none();
            })
        .def(
            "custom_layer_data_key_in_any_layer",
            [](const freeusd::usd::Stage& stage, const std::string& key) {
              return stage.CustomLayerDataKeyInAnyLayer(key);
            })
        .def(
            "list_composed_custom_layer_data_keys",
            [](const freeusd::usd::Stage& stage) { return stage.ListComposedCustomLayerDataKeys(); })
        .def(
            "get_composed_prim_variant_selection",
            [](const freeusd::usd::Stage& stage, const freeusd::sdf::Path& path, const std::string& variant_set) -> py::object {
              std::string name;
              if (stage.GetComposedPrimVariantSelection(path, variant_set, &name)) {
                return py::str(name);
              }
              return py::none();
            })
        .def(
            "prim_variant_selection_set_in_any_layer",
            [](const freeusd::usd::Stage& stage, const freeusd::sdf::Path& path, const std::string& variant_set) {
              return stage.PrimVariantSelectionSetInAnyLayer(path, variant_set);
            })
        .def(
            "list_composed_prim_variant_selection_sets",
            [](const freeusd::usd::Stage& stage, const freeusd::sdf::Path& path) {
              return stage.ListComposedPrimVariantSelectionSets(path);
            })
        .def(
            "list_composed_prim_variant_set_names",
            [](const freeusd::usd::Stage& stage, const freeusd::sdf::Path& path) {
              return stage.ListComposedPrimVariantSetNames(path);
            })
        .def(
            "prim_variant_set_declared_in_any_layer",
            [](const freeusd::usd::Stage& stage, const freeusd::sdf::Path& path, const std::string& set_name) {
              return stage.PrimVariantSetDeclaredInAnyLayer(path, set_name);
            })
        .def(
            "get_composed_prim_variant_names",
            [](const freeusd::usd::Stage& stage, const freeusd::sdf::Path& path, const std::string& set_name) {
              return stage.GetComposedPrimVariantNames(path, set_name);
            })
        .def(
            "get_composed_relocate_target",
            [](const freeusd::usd::Stage& stage, const freeusd::sdf::Path& from) -> py::object {
              freeusd::sdf::Path to;
              if (stage.GetComposedRelocateTarget(from, &to)) {
                return py::cast(to);
              }
              return py::none();
            })
        .def(
            "relocate_source_in_any_layer",
            [](const freeusd::usd::Stage& stage, const freeusd::sdf::Path& from) {
              return stage.RelocateSourceInAnyLayer(from);
            })
        .def("list_composed_relocates", [](const freeusd::usd::Stage& stage) { return stage.ListComposedRelocates(); })
        .def(
            "get_composed_prefix_substitution",
            [](const freeusd::usd::Stage& stage, const std::string& from_p) -> py::object {
              std::string to;
              if (stage.GetComposedPrefixSubstitution(from_p, &to)) {
                return py::cast(to);
              }
              return py::none();
            })
        .def(
            "prefix_substitution_key_in_any_layer",
            [](const freeusd::usd::Stage& stage, const std::string& from_p) {
              return stage.PrefixSubstitutionKeyInAnyLayer(from_p);
            })
        .def(
            "list_composed_prefix_substitutions",
            [](const freeusd::usd::Stage& stage) { return stage.ListComposedPrefixSubstitutions(); })
        .def("list_composed_field_names", &freeusd::usd::Stage::ListComposedFieldNames)
        .def("list_composed_field_sample_times", &freeusd::usd::Stage::ListComposedFieldSampleTimes)
        .def("list_composed_relationship_names", &freeusd::usd::Stage::ListComposedRelationshipNames)
        .def("list_composed_prim_paths", &freeusd::usd::Stage::ListComposedPrimPaths)
        .def("has_default_prim", &freeusd::usd::Stage::HasDefaultPrim)
        .def(
            "default_prim_name",
            [](const freeusd::usd::Stage& s) -> py::object {
              if (!s.HasDefaultPrim()) {
                return py::none();
              }
              return py::cast(s.GetDefaultPrimName());
            })
        .def(
            "default_prim",
            [](const freeusd::usd::Stage& s) -> py::object {
              const freeusd::usd::Prim p = s.GetDefaultPrim();
              if (!p.IsValid()) {
                return py::none();
              }
              return py::cast(p);
            })
        .def(
            "start_time_code",
            [](const freeusd::usd::Stage& s) -> py::object {
              const auto v = s.GetStartTimeCode();
              return v.has_value() ? py::cast(*v) : py::none();
            })
        .def(
            "end_time_code",
            [](const freeusd::usd::Stage& s) -> py::object {
              const auto v = s.GetEndTimeCode();
              return v.has_value() ? py::cast(*v) : py::none();
            })
        .def(
            "time_codes_per_second",
            [](const freeusd::usd::Stage& s) -> py::object {
              const auto v = s.GetTimeCodesPerSecond();
              return v.has_value() ? py::cast(*v) : py::none();
            })
        .def(
            "frames_per_second",
            [](const freeusd::usd::Stage& s) -> py::object {
              const auto v = s.GetFramesPerSecond();
              return v.has_value() ? py::cast(*v) : py::none();
            })
        .def(
            "frame_precision",
            [](const freeusd::usd::Stage& s) -> py::object {
              const auto v = s.GetFramePrecision();
              return v.has_value() ? py::cast(*v) : py::none();
            })
        .def(
            "meters_per_unit",
            [](const freeusd::usd::Stage& s) -> py::object {
              const auto v = s.GetMetersPerUnit();
              return v.has_value() ? py::cast(*v) : py::none();
            })
        .def(
            "up_axis",
            [](const freeusd::usd::Stage& s) -> py::object {
              const auto v = s.GetUpAxis();
              return v.has_value() ? py::cast(*v) : py::none();
            })
        .def("prim_order", [](const freeusd::usd::Stage& s) { return s.GetPrimOrder(); })
        .def("children", &freeusd::usd::Stage::GetChildren)
        .def("prim_at", &freeusd::usd::Stage::GetPrimAtPath)
        .def("prim_path_in_use", &freeusd::usd::Stage::PrimPathInUse, py::arg("path"))
        .def(
            "traverse_preorder",
            [](const freeusd::usd::Stage& st, py::object visitor) {
              st.TraversePreorder([&visitor](const freeusd::usd::Prim& p) {
                py::object r = visitor(py::cast(p));
                if (r.is_none()) {
                  return true;
                }
                return py::cast<bool>(r);
              });
            },
            py::arg("visitor"));

    py::class_<freeusd::usd::Prim>(usd, "Prim")
        .def(py::init<>())
        .def("is_valid", &freeusd::usd::Prim::IsValid)
        .def("path", &freeusd::usd::Prim::GetPath)
        .def("name", &freeusd::usd::Prim::GetName)
        .def(
            "stage",
            [](const freeusd::usd::Prim& p) -> py::object {
              const auto s = std::const_pointer_cast<freeusd::usd::Stage>(p.GetStage());
              if (!s) {
                return py::none();
              }
              return py::cast(s);
            })
        .def("parent", &freeusd::usd::Prim::GetParent)
        .def("children", &freeusd::usd::Prim::GetChildren)
        .def("has_attribute", &freeusd::usd::Prim::HasAttribute)
        .def(
            "get_attribute",
            [](const freeusd::usd::Prim& prim, const freeusd::tf::Token& name, double time) {
              return prim.GetAttribute(name, time);
            },
            py::arg("name"),
            py::arg("time") = 1.0)
        .def(
            "read_field_at_time",
            [](const freeusd::usd::Prim& prim, const freeusd::tf::Token& name, double time) -> py::object {
              const auto st = prim.GetStage();
              if (!st) {
                return py::none();
              }
              return freeusd_py_composed_read::read_field_at_time(*st, prim.GetPath(), name, time);
            },
            py::arg("name"),
            py::arg("time") = 1.0)
        .def(
            "read_field_double",
            [](const freeusd::usd::Prim& prim, const freeusd::tf::Token& name, double time) -> py::object {
              const auto st = prim.GetStage();
              if (!st) {
                return py::none();
              }
              return freeusd_py_composed_read::read_field_double(*st, prim.GetPath(), name, time);
            },
            py::arg("name"),
            py::arg("time") = 1.0)
        .def(
            "read_field_float",
            [](const freeusd::usd::Prim& prim, const freeusd::tf::Token& name, double time) -> py::object {
              const auto st = prim.GetStage();
              if (!st) {
                return py::none();
              }
              return freeusd_py_composed_read::read_field_float(*st, prim.GetPath(), name, time);
            },
            py::arg("name"),
            py::arg("time") = 1.0)
        .def(
            "read_field_bool",
            [](const freeusd::usd::Prim& prim, const freeusd::tf::Token& name, double time) -> py::object {
              const auto st = prim.GetStage();
              if (!st) {
                return py::none();
              }
              return freeusd_py_composed_read::read_field_bool(*st, prim.GetPath(), name, time);
            },
            py::arg("name"),
            py::arg("time") = 1.0)
        .def(
            "read_field_int64",
            [](const freeusd::usd::Prim& prim, const freeusd::tf::Token& name, double time) -> py::object {
              const auto st = prim.GetStage();
              if (!st) {
                return py::none();
              }
              return freeusd_py_composed_read::read_field_int64(*st, prim.GetPath(), name, time);
            },
            py::arg("name"),
            py::arg("time") = 1.0)
        .def(
            "read_field_string",
            [](const freeusd::usd::Prim& prim, const freeusd::tf::Token& name, double time) -> py::object {
              const auto st = prim.GetStage();
              if (!st) {
                return py::none();
              }
              return freeusd_py_composed_read::read_field_string(*st, prim.GetPath(), name, time);
            },
            py::arg("name"),
            py::arg("time") = 1.0)
        .def(
            "read_field_vec3d",
            [](const freeusd::usd::Prim& prim, const freeusd::tf::Token& name, double time) -> py::object {
              const auto st = prim.GetStage();
              if (!st) {
                return py::none();
              }
              return freeusd_py_composed_read::read_field_vec3d(*st, prim.GetPath(), name, time);
            },
            py::arg("name"),
            py::arg("time") = 1.0)
        .def(
            "read_field_vec3f",
            [](const freeusd::usd::Prim& prim, const freeusd::tf::Token& name, double time) -> py::object {
              const auto st = prim.GetStage();
              if (!st) {
                return py::none();
              }
              return freeusd_py_composed_read::read_field_vec3f(*st, prim.GetPath(), name, time);
            },
            py::arg("name"),
            py::arg("time") = 1.0)
        .def(
            "read_field_matrix4d",
            [](const freeusd::usd::Prim& prim, const freeusd::tf::Token& name, double time) -> py::object {
              const auto st = prim.GetStage();
              if (!st) {
                return py::none();
              }
              return freeusd_py_composed_read::read_field_matrix4d(*st, prim.GetPath(), name, time);
            },
            py::arg("name"),
            py::arg("time") = 1.0)
        .def(
            "read_field_quatd",
            [](const freeusd::usd::Prim& prim, const freeusd::tf::Token& name, double time) -> py::object {
              const auto st = prim.GetStage();
              if (!st) {
                return py::none();
              }
              return freeusd_py_composed_read::read_field_quatd(*st, prim.GetPath(), name, time);
            },
            py::arg("name"),
            py::arg("time") = 1.0)
        .def(
            "read_field_quatf",
            [](const freeusd::usd::Prim& prim, const freeusd::tf::Token& name, double time) -> py::object {
              const auto st = prim.GetStage();
              if (!st) {
                return py::none();
              }
              return freeusd_py_composed_read::read_field_quatf(*st, prim.GetPath(), name, time);
            },
            py::arg("name"),
            py::arg("time") = 1.0)
        .def(
            "read_field_token",
            [](const freeusd::usd::Prim& prim, const freeusd::tf::Token& name, double time) -> py::object {
              const auto st = prim.GetStage();
              if (!st) {
                return py::none();
              }
              return freeusd_py_composed_read::read_field_token(*st, prim.GetPath(), name, time);
            },
            py::arg("name"),
            py::arg("time") = 1.0)
        .def(
            "read_field_token_array",
            [](const freeusd::usd::Prim& prim, const freeusd::tf::Token& name, double time) -> py::object {
              const auto st = prim.GetStage();
              if (!st) {
                return py::none();
              }
              return freeusd_py_composed_read::read_field_token_array(*st, prim.GetPath(), name, time);
            },
            py::arg("name"),
            py::arg("time") = 1.0)
        .def("list_attribute_names", &freeusd::usd::Prim::ListAttributeNames)
        .def("list_relationship_names", &freeusd::usd::Prim::ListRelationshipNames)
        .def("list_attribute_sample_times", &freeusd::usd::Prim::ListAttributeSampleTimes)
        .def("has_attribute_connection", &freeusd::usd::Prim::HasAttributeConnection)
        .def(
            "get_attribute_connection_target",
            [](const freeusd::usd::Prim& prim, const freeusd::tf::Token& attr) -> py::object {
              freeusd::sdf::Path tgt;
              if (prim.GetAttributeConnectionTarget(attr, &tgt)) {
                return py::cast(tgt);
              }
              return py::none();
            })
        .def("has_relationship", &freeusd::usd::Prim::HasRelationship)
        .def("get_relationship_targets", &freeusd::usd::Prim::GetRelationshipTargets)
        .def("get_references", &freeusd::usd::Prim::GetReferences)
        .def("has_references", &freeusd::usd::Prim::HasReferences)
        .def("get_inherits", &freeusd::usd::Prim::GetInherits)
        .def("has_inherits", &freeusd::usd::Prim::HasInherits)
        .def("get_specializes", &freeusd::usd::Prim::GetSpecializes)
        .def("has_specializes", &freeusd::usd::Prim::HasSpecializes)
        .def("get_payloads", &freeusd::usd::Prim::GetPayloads)
        .def("has_payloads", &freeusd::usd::Prim::HasPayloads)
        .def("get_prim_kind", &freeusd::usd::Prim::GetPrimKind)
        .def("has_prim_kind", &freeusd::usd::Prim::HasPrimKind)
        .def("specifier_kind", &freeusd::usd::Prim::GetSpecifierKind)
        .def("is_abstract", &freeusd::usd::Prim::IsAbstract)
        .def("is_active", &freeusd::usd::Prim::IsActive)
        .def("has_prim_active_opinion", &freeusd::usd::Prim::HasPrimActiveOpinion)
        .def("has_custom_data_key", &freeusd::usd::Prim::HasCustomDataKey)
        .def(
            "get_custom_data",
            [](const freeusd::usd::Prim& prim, const std::string& key) -> py::object {
              freeusd::vt::Value v = prim.GetCustomData(key);
              if (v.IsEmpty()) {
                return py::none();
              }
              return py::cast(v);
            })
        .def("list_custom_data_keys", &freeusd::usd::Prim::ListCustomDataKeys)
        .def("has_variant_selection_key", &freeusd::usd::Prim::HasVariantSelectionKey)
        .def("get_variant_selection", &freeusd::usd::Prim::GetVariantSelection)
        .def("list_variant_selection_sets", &freeusd::usd::Prim::ListVariantSelectionSets)
        .def("has_variant_set", &freeusd::usd::Prim::HasVariantSet)
        .def("list_variant_set_names", &freeusd::usd::Prim::ListVariantSetNames)
        .def("list_variant_names", &freeusd::usd::Prim::ListVariantNames);
  }

  {
    auto geom = m.def_submodule("usdGeom");
    auto geom_tokens = geom.def_submodule("tokens");
#include "generated/usdGeom_tokens.inc"
    py::class_<freeusd::usdGeom::Imageable>(geom, "Imageable")
        .def(py::init<const freeusd::usd::Prim&>(), py::arg("prim"))
        .def_readonly("prim", &freeusd::usdGeom::Imageable::prim)
        .def("compute_visibility", &freeusd::usdGeom::Imageable::ComputeVisibility, py::arg("time") = 1.0)
        .def("compute_purpose", &freeusd::usdGeom::Imageable::ComputePurpose, py::arg("time") = 1.0);
    py::class_<freeusd::usdGeom::Boundable>(geom, "Boundable")
        .def(py::init<const freeusd::usd::Prim&>(), py::arg("prim"))
        .def_readonly("prim", &freeusd::usdGeom::Boundable::prim)
        .def("compute_local_bound", &freeusd::usdGeom::Boundable::ComputeLocalBound, py::arg("time") = 1.0)
        .def("compute_world_bound", &freeusd::usdGeom::Boundable::ComputeWorldBound, py::arg("time") = 1.0);
    py::class_<freeusd::usdGeom::Xformable>(geom, "Xformable")
        .def(py::init<const freeusd::usd::Prim&>(), py::arg("prim"))
        .def_readonly("prim", &freeusd::usdGeom::Xformable::prim)
        .def("compute_local_transform", &freeusd::usdGeom::Xformable::ComputeLocalTransform, py::arg("time") = 1.0)
        .def(
            "compute_local_to_world_transform",
            &freeusd::usdGeom::Xformable::ComputeLocalToWorldTransform,
            py::arg("time") = 1.0)
        .def(
            "compute_parent_to_world_transform",
            &freeusd::usdGeom::Xformable::ComputeParentToWorldTransform,
            py::arg("time") = 1.0);
  }

  {
    auto usdShade = m.def_submodule("usdShade");
    auto shade_tokens = usdShade.def_submodule("tokens");
#include "generated/usdShade_tokens.inc"
  }

  {
    auto usdLux = m.def_submodule("usdLux");
    auto lux_tokens = usdLux.def_submodule("tokens");
#include "generated/usdLux_tokens.inc"
  }

  {
    auto usdPhysics = m.def_submodule("usdPhysics");
    auto physics_tokens = usdPhysics.def_submodule("tokens");
#include "generated/usdPhysics_tokens.inc"
  }

  {
    auto usdVol = m.def_submodule("usdVol");
    auto vol_tokens = usdVol.def_submodule("tokens");
#include "generated/usdVol_tokens.inc"
  }

  {
    auto usdSkel = m.def_submodule("usdSkel");
    auto skel_tokens = usdSkel.def_submodule("tokens");
#include "generated/usdSkel_tokens.inc"
  }

  {
    auto usdMedia = m.def_submodule("usdMedia");
    auto media_tokens = usdMedia.def_submodule("tokens");
#include "generated/usdMedia_tokens.inc"
  }

  {
    auto usdRender = m.def_submodule("usdRender");
    auto render_tokens = usdRender.def_submodule("tokens");
#include "generated/usdRender_tokens.inc"
  }

  {
    auto usdRi = m.def_submodule("usdRi");
    auto ri_tokens = usdRi.def_submodule("tokens");
#include "generated/usdRi_tokens.inc"
  }

  {
    auto usdHydra = m.def_submodule("usdHydra");
    auto hydra_tokens = usdHydra.def_submodule("tokens");
#include "generated/usdHydra_tokens.inc"
  }

  {
    auto usdMtlx = m.def_submodule("usdMtlx");
    auto mtlx_tokens = usdMtlx.def_submodule("tokens");
#include "generated/usdMtlx_tokens.inc"
  }

  {
    auto usdProc = m.def_submodule("usdProc");
    auto proc_tokens = usdProc.def_submodule("tokens");
#include "generated/usdProc_tokens.inc"
  }

  {
    auto usdSemantics = m.def_submodule("usdSemantics");
    auto semantics_tokens = usdSemantics.def_submodule("tokens");
#include "generated/usdSemantics_tokens.inc"
  }

  {
    auto usdUI = m.def_submodule("usdUI");
    auto ui_tokens = usdUI.def_submodule("tokens");
#include "generated/usdUI_tokens.inc"
  }

  {
    auto usdUtils = m.def_submodule("usdUtils");
    usdUtils.doc() = "UsdUtils-shaped utilities (clean-room; narrow stage flattening helper plus option flags).";
    py::class_<freeusd::usdUtils::FlattenOptions>(usdUtils, "FlattenOptions")
        .def(py::init<>())
        .def_readwrite("merge_authored_layer_metadata",
                       &freeusd::usdUtils::FlattenOptions::merge_authored_layer_metadata)
        .def_readwrite("flatten_contribution_sublayers",
                       &freeusd::usdUtils::FlattenOptions::flatten_contribution_sublayers)
        .def_readwrite("preserve_relative_asset_paths",
                       &freeusd::usdUtils::FlattenOptions::preserve_relative_asset_paths);
    usdUtils.def(
        "flatten_stage_at_time",
        &freeusd::usdUtils::FlattenStageAtTime,
        py::arg("stage"),
        py::arg("time") = 1.0,
        py::arg("options") = freeusd::usdUtils::FlattenOptions{});
  }

  {
    auto work = m.def_submodule("work");
    work.doc() = "WorkDispatcher-shaped serial dispatcher (clean-room).";
    work.def(
        "run",
        [](py::function f) {
          freeusd::work::Dispatcher::Get().Run([f]() {
            if (f) {
              f();
            }
          });
        },
        py::arg("job"));
  }

  {
    auto trace = m.def_submodule("trace");
    trace.doc() = "Lightweight trace stack (depth counter; clean-room).";
    trace.def(
        "push",
        [](const std::string& tag) {
          freeusd::trace::Collector::Get().Push(tag.empty() ? nullptr : tag.c_str());
        },
        py::arg("tag") = std::string{});
    trace.def("pop", []() { freeusd::trace::Collector::Get().Pop(); });
    trace.def("stack_depth", []() { return freeusd::trace::Collector::Get().StackDepth(); });
    trace.def("reset", []() { freeusd::trace::Collector::Get().Reset(); });
  }

  {
    auto plug = m.def_submodule("plug");
    plug.def("load_all", [] { freeusd::plug::Registry::Get().LoadAllPlugins(); });
    plug.def(
        "register_plugin_paths",
        [](std::vector<std::string> paths) { freeusd::plug::Registry::Get().RegisterPluginPaths(std::move(paths)); },
        py::arg("paths"));
    plug.def("registered_plugin_paths", []() { return freeusd::plug::Registry::Get().RegisteredPluginPaths(); });
  }
}
