#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <sstream>

#include "freeusd/ar/defaultResolver.hpp"
#include "freeusd/io/usda.hpp"
#include "freeusd/pcp/layerStack.hpp"
#include "freeusd/plug/registry.hpp"
#include "freeusd/sdf/layer.hpp"
#include "freeusd/sdf/path.hpp"
#include "freeusd/tf/token.hpp"
#include "freeusd/usd/stage.hpp"
#include "freeusd/usdGeom/tokens.hpp"
#include "freeusd/version.hpp"
#include "freeusd/vt/value.hpp"

namespace py = pybind11;

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
    auto vt = m.def_submodule("vt");
    py::class_<freeusd::vt::Value>(vt, "Value")
        .def(py::init<>())
        .def_static("make_bool", &freeusd::vt::Value::MakeBool)
        .def_static("make_int32", &freeusd::vt::Value::MakeInt32)
        .def_static("make_double", &freeusd::vt::Value::MakeDouble)
        .def_static("make_string", &freeusd::vt::Value::MakeString)
        .def_static("make_token", &freeusd::vt::Value::MakeToken)
        .def("is_empty", &freeusd::vt::Value::IsEmpty)
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
        .def("append_child", [](const freeusd::sdf::Path& p, const freeusd::tf::Token& n) { return p.AppendChild(n); })
        .def("__eq__", [](const freeusd::sdf::Path& a, const freeusd::sdf::Path& b) { return a == b; })
        .def("__repr__", [](const freeusd::sdf::Path& p) { return "<freeusd.sdf.Path '" + p.GetText() + "'>"; });

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
            "set_sub_layers",
            [](freeusd::sdf::Layer& layer, std::vector<std::string> p) {
              layer.SetSubLayers(std::move(p));
            })
        .def(
            "sub_layers",
            [](const freeusd::sdf::Layer& layer) { return layer.GetSubLayers(); })
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
        .def("clear", &freeusd::sdf::Layer::Clear)
        .def("set_prim_kind", &freeusd::sdf::Layer::SetPrimKind)
        .def("has_prim_kind", &freeusd::sdf::Layer::HasPrimKind)
        .def("get_prim_kind", &freeusd::sdf::Layer::GetPrimKind)
        .def(
            "add_reference",
            [](freeusd::sdf::Layer& layer, const freeusd::sdf::Path& p, std::string a) {
              layer.AddReference(p, std::move(a));
            })
        .def(
            "list_references",
            [](const freeusd::sdf::Layer& layer, const freeusd::sdf::Path& p) { return layer.ListReferences(p); })
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
        .def(
            "get_prim_specifier",
            [](const freeusd::sdf::Layer& layer, const freeusd::sdf::Path& p) {
              return layer.GetPrimSpecifier(p);
            })
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
    py::class_<freeusd::ar::DefaultResolver>(ar, "DefaultResolver")
        .def(py::init<std::string>(), py::arg("anchor_directory") = std::string{})
        .def("resolve", [](freeusd::ar::DefaultResolver& r, const std::string& p) { return r.Resolve(p); });
  }

  {
    auto pcp = m.def_submodule("pcp");
    py::class_<freeusd::pcp::LayerStack>(pcp, "LayerStack")
        .def(py::init<>())
        .def("append", &freeusd::pcp::LayerStack::Append)
        .def("is_empty", &freeusd::pcp::LayerStack::IsEmpty)
        .def(
            "layers",
            [](const freeusd::pcp::LayerStack& s) { return s.GetLayers(); },
            py::return_value_policy::reference_internal);
  }

  {
    auto usd = m.def_submodule("usd");
    py::class_<freeusd::usd::Stage, std::shared_ptr<freeusd::usd::Stage>>(usd, "Stage")
        .def_static("attach_root_layer", &freeusd::usd::Stage::AttachRootLayer)
        .def("pseudo_root_path", &freeusd::usd::Stage::GetPseudoRootPath)
        .def("children", &freeusd::usd::Stage::GetChildren)
        .def("prim_at", &freeusd::usd::Stage::GetPrimAtPath);

    py::class_<freeusd::usd::Prim>(usd, "Prim")
        .def(py::init<>())
        .def("is_valid", &freeusd::usd::Prim::IsValid)
        .def("path", &freeusd::usd::Prim::GetPath)
        .def("has_attribute", &freeusd::usd::Prim::HasAttribute)
        .def("get_attribute", &freeusd::usd::Prim::GetAttribute);
  }

  {
    auto geom = m.def_submodule("usdGeom");
    geom.def("token_mesh", [] { return freeusd::usdGeom::tokens::Mesh(); });
    geom.def("token_xform", [] { return freeusd::usdGeom::tokens::Xform(); });
    geom.def("token_scope", [] { return freeusd::usdGeom::tokens::Scope(); });
  }

  {
    auto plug = m.def_submodule("plug");
    plug.def("load_all", [] { freeusd::plug::Registry::Get().LoadAllPlugins(); });
  }
}
