# Integrating FreeUSD into a C++ project (including game engines)

This document is the engine-embedding guide. Pair it with:

- [engine-supported-subset.md](engine-supported-subset.md) for the frozen engine contract
- [openusd-parity-matrix.md](openusd-parity-matrix.md) for what is actually implemented
- [openusd-map.md](openusd-map.md) for the broader module map

## Licensing

FreeUSD library sources are **GPL-3.0-or-later**. The project is aimed especially at **GPL-3-compatible** applications such as GPL engines. Proprietary engines that cannot adopt GPLv3 for linked code still need a different USD path or a separate license.

## Recommended Integration Model

- **Tools and editor modules:** link `freeusd::runtime` and use the C++ APIs directly.
- **Runtime-facing bridges:** prefer `freeusd::c` as the stable FFI boundary.
- **Primary interchange:** `USDA`.
- **Shipping runtime default:** `pre_baked_assets_only`.

## Engine Contract

The authoritative engine subset lives in [engine-supported-subset.md](engine-supported-subset.md). In short:

- validated scene import/editor work is `usdGeom`-centric
- validated composition arcs are limited to the fixture-backed subset
- arbitrary `.usdc` scene loading is **not** supported yet
- the only validated crate scene-open path today is a narrow embedded-`USDA` fallback used for controlled fixtures and pipeline experiments

## CMake Consumption

### In-tree (`add_subdirectory`)

```cmake
add_subdirectory(third_party/freeusd EXCLUDE_FROM_ALL)

target_link_libraries(engine_importer PRIVATE freeusd::runtime)
target_link_libraries(engine_editor PRIVATE freeusd::runtime)
target_link_libraries(engine_runtime_bridge PRIVATE freeusd::c)
```

`freeusd::runtime` is the single-link C++ aggregate. `freeusd::c` is the stable C ABI wrapper over that runtime stack.

### Installed package

```cmake
list(APPEND CMAKE_PREFIX_PATH "/path/to/freeusd/prefix")
find_package(FreeUSD REQUIRED)
target_link_libraries(engine_importer PRIVATE freeusd::runtime)
target_link_libraries(engine_runtime_bridge PRIVATE freeusd::c)
```

## Approved Public APIs

### C++ surfaces for tools/editor

- `freeusd::usd::Stage`
- `freeusd::usd::Prim`
- `freeusd::usdGeom::Xformable`
- `freeusd::usdGeom::Imageable`
- `freeusd::usdGeom::Boundable`
- `freeusd::usdUtils::FlattenStageAtTime`
- `freeusd::usdUtils::BuildEngineSceneSnapshot`
- `freeusd::usdUtils::BuildEnginePrimEditorView`
- `freeusd::usdUtils::AssessEngineRuntimeSupport`

### C ABI surfaces for runtime bridges

- stage open/query helpers in [`include/freeusd/c/freeusd.h`](../include/freeusd/c/freeusd.h)
- typed field reads and sample-time queries
- composition arc queries
- engine runtime support assessment (`freeusd_usdutils_assess_engine_runtime_support`)
- `freeusd_stage_compute_local_transform_matrix4d`
- `freeusd_stage_compute_local_to_world_transform_matrix4d`
- `freeusd_stage_compute_imageable_visibility`
- `freeusd_stage_compute_imageable_purpose_utf8`
- `freeusd_stage_compute_boundable_local_bounds`
- `freeusd_stage_compute_boundable_world_bounds`

## Build Options Relevant To Engines

| CMake option | Default | Notes |
| --- | --- | --- |
| `FREEUSD_BUILD_PYTHON` | `ON` | Turn `OFF` for minimal engine integrations. |
| `FREEUSD_BUILD_TESTS` | `ON` | Keep `ON` in CI; optional for shipped engine builds. |
| `FREEUSD_BUILD_C_ABI` | `ON` | Keep `ON` when runtime bridges use `freeusd::c`. |
| `FREEUSD_TEST_INSTALL_INTEGRATION` | `OFF` | Extra install/find-package smoke test. |

## Threading

- `Stage`, `Layer`, and related mutable objects are not implicitly synchronized.
- Treat both C++ and C ABI objects as single-owner unless the engine adds external locking.
- `freeusd_last_error_message` is thread-local, but stage/layer access is not.

## Formats

- **USDA:** supported and recommended.
- **USDC low-level access:** bootstrap, TOC, raw section bytes, and validated `TOKENS` / `STRINGS` / `PATHS` / `FIELDS` table decode exist.
- **USDC scene open:** only the narrow embedded-`USDA` section fallback is validated today. It advances controlled engine pipelines, but it is not general `.usdc` parity.

## Validation Anchors

- hierarchy / field reads: `tests/fixtures/usd_cross_language.usda`
- sublayers / offsets / sample times: `tests/fixtures/parity_stack_root.usda`
- references / payloads / relocates / prefix substitutions: `tests/fixtures/parity_namespace.usda`
- composed prim kind / active through arcs: `tests/fixtures/parity_kind_active_refs.usda`
- composed prim kind / active through specializes: `tests/fixtures/parity_kind_active_specializes.usda`
- composed prim customData through inherits: `tests/fixtures/parity_custom_data_inherit.usda`
- composed prim customData through references / payloads: `tests/fixtures/parity_custom_data_refs.usda`
- variants: `tests/fixtures/parity_variants.usda`
- transforms / visibility / purpose / bounds: `tests/fixtures/parity_imageable.usda`
- preview materials / bindings: `tests/fixtures/parity_shade_preview.usda`
- preview-surface texture assets: `tests/fixtures/parity_shade_pbr_textures.usda`
- usdLux light families: `tests/fixtures/parity_lux_sphere.usda` (and sibling `parity_lux_*.usda` fixtures)
- PhysicsScene gravity inputs: `tests/fixtures/parity_physics_scene.usda`
- crate tables: `tests/fixtures/parity_tables.usdc`
- narrow crate scene-open fallback: `tests/fixtures/parity_embedded_scene.usdc`

## Shipping Checklist

1. Confirm the engine stays inside [engine-supported-subset.md](engine-supported-subset.md).
2. Keep the first production import path USDA-first.
3. Use `AssessEngineRuntimeSupport` to decide whether a scene must stay pre-baked.
4. Do not ship arbitrary live `.usdc` stage loading.
5. Keep editor/runtime claims aligned with [openusd-parity-matrix.md](openusd-parity-matrix.md).
