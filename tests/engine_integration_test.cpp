#include <cassert>
#include <string>

#include "freeusd/sdf/path.hpp"
#include "freeusd/usd/stage.hpp"
#include "freeusd/usdUtils/engineScene.hpp"

#ifndef FREEUSD_TEST_FIXTURES_DIR
#error "FREEUSD_TEST_FIXTURES_DIR must be set by CMake for engine_integration_test"
#endif

namespace {

std::string fixture(const char* name) {
  return std::string(FREEUSD_TEST_FIXTURES_DIR) + "/" + name;
}

const freeusd::usdUtils::EngineSceneNode* find_node(const freeusd::usdUtils::EngineSceneSnapshot& snapshot,
                                                    const freeusd::sdf::Path& path) {
  for (const auto& node : snapshot.nodes) {
    if (node.path == path) {
      return &node;
    }
  }
  return nullptr;
}

}  // namespace

int main() {
  using freeusd::sdf::Path;
  using freeusd::usd::RootLayerSublayersPolicy;
  using freeusd::usd::Stage;
  using freeusd::usdUtils::AssessEngineRuntimeSupport;
  using freeusd::usdUtils::BuildEnginePrimEditorView;
  using freeusd::usdUtils::BuildEngineSceneSnapshot;
  using freeusd::usdUtils::EngineRuntimeMode;
  using freeusd::usdUtils::EngineRuntimeModeName;

  {
    std::string err;
    auto stage = Stage::OpenFromRootFile(fixture("parity_imageable.usda"), RootLayerSublayersPolicy::DepthFirst, &err);
    assert(stage && err.empty());
    const auto snapshot = BuildEngineSceneSnapshot(*stage, 1.0);
    assert(snapshot.default_prim_name == "World");
    assert(snapshot.root_identifier.find("parity_imageable.usda") != std::string::npos);
    const auto* world = find_node(snapshot, Path::FromString("/World"));
    const auto* cube = find_node(snapshot, Path::FromString("/World/Cube"));
    assert(world != nullptr);
    assert(cube != nullptr);
    assert(world->child_paths.size() == 1u);
    assert(world->child_paths[0] == Path::FromString("/World/Cube"));
    assert(cube->purpose == "render");
    assert(!cube->visible);
    assert(!cube->world_bound.IsEmpty());
    assert(cube->world_bound.min.x() == 0.0 && cube->world_bound.min.y() == 1.0 && cube->world_bound.min.z() == 2.0);
    assert(cube->world_bound.max.x() == 2.0 && cube->world_bound.max.y() == 3.0 && cube->world_bound.max.z() == 4.0);

    const auto report = AssessEngineRuntimeSupport(*stage);
    assert(report.recommended_mode == EngineRuntimeMode::ExperimentalLiveStage);
    assert(std::string(EngineRuntimeModeName(report.recommended_mode)) == "experimental_live_stage");
  }

  {
    std::string err;
    auto stage = Stage::OpenFromRootFile(fixture("parity_stack_root.usda"), RootLayerSublayersPolicy::DepthFirst, &err);
    assert(stage && err.empty());
    const auto prim = stage->GetPrimAtPath(Path::FromString("/World/Model"));
    const auto editor = BuildEnginePrimEditorView(prim, 15.0);
    assert(editor.path == Path::FromString("/World/Model"));
    bool saw_stacked_only = false;
    for (const auto& name : editor.composed_field_names) {
      if (name == "stackedOnly") {
        saw_stacked_only = true;
        break;
      }
    }
    assert(saw_stacked_only);
    bool saw_stacked_only_samples = false;
    for (const auto& item : editor.attribute_sample_times) {
      if (item.first == "stackedOnly") {
        saw_stacked_only_samples = true;
        assert(item.second.size() == 2u);
        assert(item.second[0] == -5.0);
        assert(item.second[1] == 15.0);
      }
    }
    assert(saw_stacked_only_samples);

    const auto report = AssessEngineRuntimeSupport(*stage);
    assert(report.uses_composed_layer_stack);
    assert(report.uses_time_samples);
    assert(report.recommended_mode == EngineRuntimeMode::PreBakedAssetsOnly);
  }

  {
    std::string err;
    auto stage = Stage::OpenFromRootFile(fixture("parity_namespace.usda"), RootLayerSublayersPolicy::DepthFirst, &err);
    assert(stage && err.empty());
    const auto report = AssessEngineRuntimeSupport(*stage);
    assert(report.uses_references);
    assert(report.uses_payloads);
    assert(report.uses_relocates);
    assert(report.uses_prefix_substitutions);
    assert(report.recommended_mode == EngineRuntimeMode::PreBakedAssetsOnly);
    assert(!report.warnings.empty());
  }

  {
    std::string err;
    auto stage = Stage::OpenFromRootFile(fixture("parity_variants.usda"), RootLayerSublayersPolicy::DepthFirst, &err);
    assert(stage && err.empty());
    const auto report = AssessEngineRuntimeSupport(*stage);
    assert(report.uses_variant_selection);
    assert(report.uses_variant_sets);
    assert(report.recommended_mode == EngineRuntimeMode::PreBakedAssetsOnly);
  }

  return 0;
}
