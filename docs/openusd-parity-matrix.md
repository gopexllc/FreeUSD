# OpenUSD Parity Matrix

This file is the repo-owned parity baseline for FreeUSD. It records what "parity" means today, which fixtures anchor that claim, and what must be true before later roadmap slices are considered complete.

Status vocabulary:

- `implemented`: exercised in tests and intended as part of the current supported subset.
- `partial`: real behavior exists, but important OpenUSD-shaped gaps remain.
- `token-only`: package/module naming or tokens exist without meaningful runtime behavior.
- `planned`: explicitly targeted next, but not yet represented as runtime behavior.
- `intentionally deferred`: visible gap that should stay out of scope until stronger core parity exists.

## Fixture Corpus

- `tests/fixtures/usd_cross_language.usda`
  Cross-language typed field-read contract and basic layer metadata.
- `tests/fixtures/parity_stack_root.usda`
  Root-file opening, sublayer expansion, strongest/weakest field opinions, and authored `subLayerOffsets`.
- `tests/fixtures/parity_stack_mid.usda`
  Mid-strength layer opinions for file-open and stack fixtures.
- `tests/fixtures/parity_stack_weak.usda`
  Weakest layer opinions for stack and time-sample union checks.
- `tests/fixtures/parity_namespace.usda`
  Namespace-affecting metadata: `relocates`, `prefixSubstitutions`, references, and payloads.
- `tests/fixtures/parity_variants.usda`
  `variantSets` declarations, selected-variant payload execution, and variant child expansion.
- `tests/fixtures/parity_imageable.usda`
  Schema-facing `purpose` / `visibility` behavior for `usdGeom::Imageable` and cube-like bounds via `usdGeom::Boundable`.
- `tests/fixtures/parity_custom_data_inherit.usda`
  Composed prim `customData` through `inherits` arcs (local strongest-wins override plus inherited keys).
- `tests/fixtures/parity_kind_active_refs.usda`
  Composed prim `kind` and `active` through `references`, `payloads`, and `inherits` (`parity_kind_active_ref.usda`, `parity_kind_active_payload.usda`).
- `tests/fixtures/parity_tables.usdc`
  Shared binary crate fixture for bootstrap, TOC, raw section payloads, and validated `TOKENS` / `STRINGS` / `PATHS` / `FIELDS` table decode.
- `tests/fixtures/parity_embedded_scene.usdc`
  Narrow crate scene-open fallback through an embedded `USDA` section for controlled engine pipelines and fixtures.
- `tests/fixtures/parity_skel_gltf.usda`
  Two-joint skeleton plus `SkelAnimation` TRS time samples for glTF skin/animation channel mapping.
- `tests/fixtures/parity_skel_gltf.usda`
  glTF-shaped skeleton + two-key `SkelAnimation` TRS channels via `usdSkel::Skeleton` and `usdSkel::SkelAnimation`.
- `tests/fixtures/parity_skel_blend_shapes.usda`
  Mesh blend shapes (`BlendShape.offsets`, `skel:blendShapes` / `skel:blendShapeTargets`, time-sampled `blendShapeWeights`) for glTF morph-target mapping.
- `tests/fixtures/parity_skel_skinning.usda`
  Single-point mesh with joint influences plus animated `SkelAnimation` for CPU linear blend skinning (`DeformPointsWithSkeleton`).
- `tests/fixtures/parity_shade_preview.usda`
  `Material` with `UsdPreviewSurface` shader, `outputs:surface` connection, and `material:binding` on a mesh.
- `tests/fixtures/parity_lux_distant.usda`
  `DistantLight` with `inputs:intensity`, `inputs:color`, and `inputs:angle`.

## Current Matrix

### Formats And Data Model

- `implemented`: USDA load/save, typed scalar/vector/quaternion/matrix values, layer metadata, references/payload/inherits/specializes storage, relationship targets, and time-sample evaluation.
- `partial`: USDC bootstrap parsing, TOC parsing, raw section-payload reads, validated `TOKENS` / `STRINGS` / `PATHS` / `FIELDS` table decode, and a narrow embedded-`USDA` stage-open fallback are available in C++; the C ABI follows the same validated open/query slice.
- `planned`: spec-level `.usdc` payload decode beyond the validated table sections and embedded-`USDA` bridge.

### Composition Semantics

- `implemented`: strongest-wins field reads, concatenated relationship lists, composed field/relationship/prim-path unions, relocated prim-path query behavior, and prefix-substituted reference/payload asset paths.
- `partial`: `subLayerOffsets` now remap composed sample times and file-backed reads; selected variants plus reference/payload/inherit/specialize arcs now affect file-backed field and prim-path queries; composed prim `kind` / `active`, `customData`, and USDA `class` / `over` specifier resolution follow references, payloads, and `inherits` / `specializes` (local layer stack still wins when authored), but other metadata propagation through every arc type remains incomplete.
- `planned`: broader resolver-aware arc expansion for the remaining composed query families.

### Schema And Runtime Helpers

- `implemented`: `usdGeom::Xformable`, `usdGeom::Imageable`, `usdGeom::Boundable`, `usdUtils::FlattenStageAtTime`, and `usdUtils` engine-scene helpers for importer/editor/runtime subset inspection.
- `partial`: flattening now preserves evaluated defaults plus composed sample times, but it does not yet reconstruct full authored layer provenance for every arc source.
- `partial`: `usdSkel::Skeleton` and `usdSkel::SkelAnimation` read joints, bind/rest matrices, and sampled TRS arrays from USDA; glTF mapping helpers build parent indices and world bind matrices; `SkelBinding` resolves `skel:skeleton` plus `primvars:skel:jointIndices` / `jointWeights`; `SkelRoot` finds skeleton and `skel:animationSource` under a scope (`parity_skel_binding.usda`); `BlendShape` / `SkelBlendShapes` / `MorphTargets` read morph offsets, remap animation weights, and apply CPU morph accumulation (`parity_skel_blend_shapes.usda`; glTF `mesh.weights` + morph target POSITION deltas); `DeformPointsWithSkeleton` performs CPU LBS from joint world matrices and inverse bind transforms (`parity_skel_skinning.usda`).
- `partial`: `usdShade::Material` resolves `outputs:surface` to a shader prim; `usdShade::Shader` / `PreviewSurface` read `info:id` and common `UsdPreviewSurface` inputs (`diffuseColor`, `metallic`, `roughness`, `opacity`) with connection following (`parity_shade_preview.usda`).
- `partial`: `usdLux::DistantLight` reads `inputs:intensity`, `inputs:color`, and `inputs:angle` at a time code (`parity_lux_distant.usda`).
- `token-only`: most other non-`usdGeom` / non-`usdSkel` / non-`usdShade` / non-`usdLux` schema packages remain generated token surfaces only.

### ABI And Bindings

- `implemented`: the C ABI remains the stable FFI contract for stage queries, typed field reads, crate bootstrap/TOC helpers, raw crate section payload access, and the validated `usdGeom` runtime subset (`transform`, `visibility`, `purpose`, `bounds`).
- `partial`: Python remains the broadest wrapper; the C ABI, Go, and Rust now follow the validated crate bootstrap/TOC/raw-section/table subset, but broader runtime/schema helpers are still not fully mirrored outside Python/C++.
- `planned`: milestone-by-milestone expansion for more composed query families and runtime helpers once the C ABI slice is stable.

### Runtime Hardening And Deferred Stacks

- `implemented`: lightweight `plug`, `trace`, and `work` utilities exist for validation and smoke coverage.
- `intentionally deferred`: Hydra/imaging, Ndr/Sdr, and broader runtime ecosystems.

### Engine Integration Contract

- `implemented`: `docs/engine-supported-subset.md`, `docs/engine-integration.md`, clean-room/fixture/claim policy docs, `usdUtils::engineScene`, and focused engine integration tests freeze the USDA-first engine path.
- `partial`: shipping runtime remains intentionally narrow; engine snapshots now list material bindings, preview-surface materials, and distant lights; `AssessEngineRuntimeSupport` reports `uses_material_bindings` / `uses_preview_surface`; arbitrary `.usdc` scene decode and broad live-stage runtime guarantees are still out of scope.

## Acceptance Criteria

### Milestone 1: Parity Baseline

- New parity work adds or reuses a named fixture from `tests/fixtures/`.
- The matrix above is updated when a status meaningfully changes.
- At least one native test and one binding-facing test cover the new behavior when an FFI surface is involved.

### Milestone 2: Formats And Data Model

- USDC work lands first in C++ and is validated with synthetic crate fixtures or checked-in files.
- The C ABI mirrors only the validated crate slice.
- Python, Go, and Rust expose the same validated low-level crate behavior before broader marketing claims change.

### Milestone 3: Composition Semantics

- New graph behavior must affect composed queries, not only stored metadata accessors.
- Relocation/prefix/path semantics must be exercised through `Stage` or `Prim`, not just `Layer`.
- Cross-language tests should verify any behavior that changes public query results.

### Milestone 4: Schema And Runtime APIs

- New schema helpers must read authored scenes and return behavior, not only tokens.
- `usdUtils` additions must be executable utilities with tests, not placeholder structs.

### Milestone 5: ABI And Binding Expansion

- C ABI additions document ownership and failure semantics in `include/freeusd/c/freeusd.h`.
- Python can go first, but Go/Rust should follow once the C ABI slice is stable and tested.

### Milestone 6: Deferred Stacks

- Imaging/Hydra-class work stays out of scope until the matrix rows above move from `partial` to stronger core parity.
