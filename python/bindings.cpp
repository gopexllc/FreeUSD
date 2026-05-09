#include <pybind11/functional.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <optional>
#include <sstream>

#include "freeusd/ar/defaultResolver.hpp"
#include "freeusd/io/usda.hpp"
#include "freeusd/pcp/compose.hpp"
#include "freeusd/pcp/layerStack.hpp"
#include "freeusd/plug/registry.hpp"
#include "freeusd/sdf/layer.hpp"
#include "freeusd/sdf/path.hpp"
#include "freeusd/sdf/primReference.hpp"
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
        .def("clear_sub_layers", &freeusd::sdf::Layer::ClearSubLayers)
        .def("has_default_prim", &freeusd::sdf::Layer::HasDefaultPrim)
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
    py::class_<freeusd::usd::Stage, std::shared_ptr<freeusd::usd::Stage>>(usd, "Stage")
        .def_static("attach_root_layer", &freeusd::usd::Stage::AttachRootLayer)
        .def_static("attach_layer_stack", &freeusd::usd::Stage::AttachLayerStack)
        .def("pseudo_root_path", &freeusd::usd::Stage::GetPseudoRootPath)
        .def(
            "compose_layers",
            [](const freeusd::usd::Stage& st) { return st.GetComposeLayers(); })
        .def(
            "read_field_at_time",
            [](const freeusd::usd::Stage& st, const freeusd::sdf::Path& p, const freeusd::tf::Token& t,
               double time) -> py::object {
              freeusd::vt::Value v;
              if (st.ReadFieldAtEvaluatedTime(p, t, time, &v)) {
                return py::cast(v);
              }
              return py::none();
            },
            py::arg("path"), py::arg("name"), py::arg("time") = 1.0)
        .def("resolve_prim_kind", &freeusd::usd::Stage::ResolvePrimKind)
        .def("resolve_has_prim_kind", &freeusd::usd::Stage::ResolveHasPrimKind)
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
        .def("children", &freeusd::usd::Stage::GetChildren)
        .def("prim_at", &freeusd::usd::Stage::GetPrimAtPath);

    py::class_<freeusd::usd::Prim>(usd, "Prim")
        .def(py::init<>())
        .def("is_valid", &freeusd::usd::Prim::IsValid)
        .def("path", &freeusd::usd::Prim::GetPath)
        .def("has_attribute", &freeusd::usd::Prim::HasAttribute)
        .def("get_attribute", &freeusd::usd::Prim::GetAttribute)
        .def("has_relationship", &freeusd::usd::Prim::HasRelationship)
        .def("get_relationship_targets", &freeusd::usd::Prim::GetRelationshipTargets)
        .def("get_prim_kind", &freeusd::usd::Prim::GetPrimKind)
        .def("has_prim_kind", &freeusd::usd::Prim::HasPrimKind)
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
        .def("list_custom_data_keys", &freeusd::usd::Prim::ListCustomDataKeys);
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
