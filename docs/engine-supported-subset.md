# Engine Supported Subset

This document freezes the exact FreeUSD subset a GPL-compatible engine may rely on today. Treat it as the engine contract for tools, editor, and runtime.

## Production Posture

- Primary interchange is `USDA`.
- `USDC` is not treated as generally supported scene input. The only validated scene-open path today is a narrow crate fallback that loads an embedded `USDA` section; use it only for controlled pipelines and fixtures, not arbitrary upstream `.usdc` files.
- The recommended shipping runtime mode is `pre_baked_assets_only`. Use `hybrid_metadata` only when a runtime system needs narrow composed metadata reads. Treat `experimental_live_stage` as opt-in and non-default.

## Approved Public APIs

### Tools And Importers

- Link `freeusd::runtime`.
- Approved C++ surfaces:
  - `freeusd::usd::Stage`
  - `freeusd::usd::Prim`
  - `freeusd::usdGeom::Xformable`
  - `freeusd::usdGeom::Imageable`
  - `freeusd::usdGeom::Boundable`
  - `freeusd::usdUtils::FlattenStageAtTime`
  - `freeusd::usdUtils::BuildEngineSceneSnapshot`
  - `freeusd::usdUtils::BuildEnginePrimEditorView`
  - `freeusd::usdUtils::AssessEngineRuntimeSupport`

### Runtime And Stable FFI Boundaries

- Prefer `freeusd::c` for runtime-facing bridges, scripting layers, and hot-reload/plugin seams.
- Approved C ABI surfaces:
  - stage open/query helpers in `include/freeusd/c/freeusd.h`
  - typed field reads and sample-time lists
  - relationship/composition arc queries
  - `freeusd_stage_compute_local_transform_matrix4d`
  - `freeusd_stage_compute_local_to_world_transform_matrix4d`
  - `freeusd_stage_compute_imageable_visibility`
  - `freeusd_stage_compute_imageable_purpose_utf8`
  - `freeusd_stage_compute_boundable_local_bounds`
  - `freeusd_stage_compute_boundable_world_bounds`

## Supported Content Slice

- USDA root layers with optional USDA sublayers.
- `usdGeom`-centric scene graphs that rely on:
  - hierarchy traversal
  - composed field inspection
  - transforms via `Xformable`
  - visibility/purpose via `Imageable`
  - cube/radius-style bounds via `Boundable`
- Composition features only where already fixture-backed:
  - references
  - payloads
  - inherits
  - specializes
  - variant selections / variant sets
  - relocates
  - prefix substitutions
  - sublayer offsets

## Validated Engine Behaviors

| Behavior | Anchor |
| --- | --- |
| scene hierarchy + typed field reads | `tests/fixtures/usd_cross_language.usda` |
| sublayer expansion + sublayer offsets + time samples | `tests/fixtures/parity_stack_root.usda` |
| references / payloads / relocates / prefix substitutions | `tests/fixtures/parity_namespace.usda` |
| selected variant expansion | `tests/fixtures/parity_variants.usda` |
| visibility / purpose / bounds / transforms | `tests/fixtures/parity_imageable.usda` |
| low-level crate tables | `tests/fixtures/parity_tables.usdc` |
| narrow crate scene-open fallback | `tests/fixtures/parity_embedded_scene.usdc` |

## Intentionally Unsupported In Shipping Runtime

- Hydra / imaging / render delegates
- Ndr / Sdr / shader-discovery stacks
- wide plugin ecosystems
- non-`usdGeom` runtime schema behavior beyond token/schema markers
- arbitrary `.usdc` scene decode
- concurrent mutation or unsynchronized multi-threaded access to one stage/layer graph

## Threading Contract

- `Stage`, `Layer`, and related mutable objects are single-owner unless the embedding application adds external locking.
- The C ABI follows the same rule.
- `freeusd_last_error_message` is thread-local, but object access is not implicitly synchronized.

## Runtime Mode Rules

### `pre_baked_assets_only`

- Default and recommended.
- Required when scenes use composed layer stacks, references, payloads, inherits, specializes, variants, relocates, prefix substitutions, or time samples.

### `hybrid_metadata`

- Allowed only when engine assets are already baked and runtime reads are limited to narrow metadata, field queries, or inspection helpers.

### `experimental_live_stage`

- Only for static scenes that stay within the validated `usdGeom` subset and do not require the higher-risk composition features above.
- Do not make this the default shipping path.
