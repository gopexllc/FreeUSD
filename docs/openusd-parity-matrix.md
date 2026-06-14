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
  `variantSets` declarations (including composed set/name lists through references), selected-variant payload execution, and variant child expansion.
- `tests/fixtures/parity_imageable.usda`
  Schema-facing `purpose` / `visibility` behavior for `usdGeom::Imageable` and cube-like bounds via `usdGeom::Boundable`.
- `tests/fixtures/parity_custom_data_inherit.usda`
  Composed prim `customData` through `inherits` arcs (local strongest-wins override plus inherited keys).
- `tests/fixtures/parity_custom_data_specializes.usda`
  Composed prim `customData` through `specializes` from a `class` prim (local override on specialize host).
- `tests/fixtures/parity_variant_selection_refs.usda`
- `tests/fixtures/parity_variant_selection_payloads.usda`
- `tests/fixtures/parity_variant_sets_refs.usda`
  Composed `variantSets` metadata and variant names through a referenced layer (`parity_variant_sets_ref.usda`).
  Composed prim `variantSelection` through `references` (`parity_variant_selection_ref.usda`) and payloads (`parity_variant_selection_payload.usda`).
  Composed prim `customData` through `specializes` from a `class` prim (local override on specialize host).
- `tests/fixtures/parity_custom_data_refs.usda`
  Composed prim `customData` through `references` and `payloads` (`parity_custom_data_ref.usda`, `parity_custom_data_payload.usda`; local override on reference host).
- `tests/fixtures/parity_specializes.usda`
  Composed attribute reads through `specializes` arcs (arc-sourced defaults plus local strongest-wins override).
- `tests/fixtures/parity_kind_active_refs.usda`
  Composed prim `kind` and `active` through `references`, `payloads`, and `inherits` (`parity_kind_active_ref.usda`, `parity_kind_active_payload.usda`).
- `tests/fixtures/parity_kind_active_specializes.usda`
  Composed prim `kind` and `active` through `specializes` from a `class` prim.
- `tests/fixtures/parity_tables.usdc`
- `tests/fixtures/parity_tables_zlib.usdc`
  Same typed `VALUES` rows as `parity_tables.usdc` with a fixture zlib wrapper (`FUSDZC`) on the `VALUES` section (regenerate with `scripts/gen_parity_compressed_usdc.py`).
- `tests/fixtures/parity_tables_lz4.usdc`
  Same typed `VALUES` rows with fixture LZ4 block wrapper (`FUSDZL`; regenerate with `scripts/gen_parity_lz4_usdc.py`; see `docs/usdc-fixture-compression.md`).
  Shared binary crate fixture for bootstrap, TOC, raw section payloads, and validated `TOKENS` / `STRINGS` / `PATHS` / `FIELDS` / `FIELDSETS` / `SPECS` / `VALUES` table decode (regenerate with `scripts/gen_parity_tables_usdc.py`).
- `tests/fixtures/parity_geom_mesh.usda`
  `UsdGeomMesh`-shaped `points`, `faceVertexCounts`, `faceVertexIndices`, `normals`, `primvars:st`, `primvars:displayOpacity`, and `primvars:displayColor` on a triangular mesh prim.
- `tests/fixtures/parity_lux_sphere.usda`
  `SphereLight` with `inputs:intensity`, `inputs:color`, and `inputs:radius`.
- `tests/fixtures/parity_lux_rect.usda`
  `RectLight` with `inputs:intensity`, `inputs:color`, `inputs:width`, and `inputs:height`.
- `tests/fixtures/parity_shade_texture.usda`
  `UsdPreviewSurface` with `inputs:diffuseColor` connected to `UsdUVTexture` `inputs:file`.
- `tests/fixtures/parity_shade_pbr_textures.usda`
  `UsdPreviewSurface` with `inputs:normal`, `inputs:occlusion`, `inputs:metallic`, and `inputs:roughness` connected to texture shaders plus authored `inputs:emissiveColor`.
- `tests/fixtures/parity_lux_disk.usda`
  `DiskLight` with `inputs:intensity`, `inputs:color`, and `inputs:radius`.
- `tests/fixtures/parity_lux_cylinder.usda`
  `CylinderLight` with `inputs:intensity`, `inputs:color`, `inputs:length`, and `inputs:radius`.
- `tests/fixtures/parity_lux_dome.usda`
  `DomeLight` with `inputs:intensity`, `inputs:color`, `inputs:texture:file`, and `inputs:texture:format`.
- `tests/fixtures/parity_embedded_scene.usdc`
- `tests/fixtures/parity_embedded_scene_zlib.usdc`
- `tests/fixtures/parity_embedded_scene_lz4.usdc`
  Embedded USDA scene with LZ4-wrapped `USDA` section (`FUSDZL`).
  Same embedded USDA scene with zlib-wrapped `USDA` section (`FUSDZC`; regenerate with `scripts/gen_parity_embedded_scene_zlib_usdc.py`).
- `scripts/gen_parity_embedded_scene_usdc.py`
  Regenerates `parity_embedded_scene.usdc` (embedded `USDA` section).
  Narrow crate scene-open fallback through an embedded `USDA` section for controlled engine pipelines and fixtures.
- `tests/fixtures/parity_skel_gltf_pipeline.usda`
  glTF-shaped single-file pipeline: `SkelRoot`, skeleton, animation, skinned mesh, and one morph target.
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
- `tests/fixtures/parity_physics_scene.usda`
  `PhysicsScene` with `physics:gravityDirection` and `physics:gravityMagnitude`.
- `tests/fixtures/parity_physics_rigid_body.usda`
  `PhysicsRigidBodyAPI` with authored `physics:mass` on an `Xform` prim.
- `tests/fixtures/parity_physics_rigid_body_inherit.usda`
  Composed `apiSchemas` and `physics:mass` through `inherits` from a `class` prim.
- `tests/fixtures/parity_physics_rigid_body_kinematic.usda`
  `PhysicsRigidBodyAPI` with `physics:mass` and `physics:kinematicEnabled` (`1` literal coerced to bool).
- `tests/fixtures/parity_physics_mass.usda`
  `PhysicsMassAPI` with `physics:density` and `vector3f` `physics:centerOfMass`.
- `tests/fixtures/parity_physics_fixed_joint.usda`
  `PhysicsFixedJoint` with `physics:body0` / `physics:body1` relationships and `physics:jointEnabled`.
- `tests/fixtures/parity_physics_rigid_body_refs.usda`
  Composed `physics:mass` through `references` (`parity_physics_rigid_body_ref.usda`).
- `tests/fixtures/parity_physics_collision.usda`
  `PhysicsCollisionAPI` with authored `physics:collisionEnabled` (`0` literal coerced to bool).
- `tests/fixtures/parity_physics_collision_inherit.usda`
  Composed `apiSchemas` and `physics:collisionEnabled` through `inherits` from a `class` prim.
- `tests/fixtures/parity_vol_openvdb.usda`
  `OpenVDBAsset` with `filePath` and `fieldName`.
- `tests/fixtures/parity_vol_volume.usda`
  `Volume` with composed `field` relationship to a child `OpenVDBAsset`.

## Current Matrix

### Formats And Data Model

- `implemented`: USDA load/save, typed scalar/vector/quaternion/matrix values (including `vector3f` tuple literals and `bool` attributes with `true`/`false` or `0`/`1` literals), layer metadata, references/payload/inherits/specializes storage, relationship targets, and time-sample evaluation.
- `partial`: USDC bootstrap parsing, TOC parsing, raw section-payload reads, validated `TOKENS` / `STRINGS` / `PATHS` / `FIELDS` / `FIELDSETS` / `SPECS` / `VALUES` table decode on `parity_tables.usdc` (fixture typed kinds through `TokenIndexArray` (19); see zlib/lz4 wrapper rows below), and a narrow embedded-`USDA` stage-open fallback are available in C++; the C ABI follows the same validated open/query slice (`freeusd_read_usdc_typed_values_table_from_path_utf8`).
- `partial`: fixture zlib (`FUSDZC`) and LZ4 block (`FUSDZL`) decompression for wrapped section payloads (`parity_tables_zlib.usdc`, `parity_tables_lz4.usdc`; see `docs/usdc-fixture-compression.md`); `FloatArray` (12), `DoubleArray` (13), `Vec2f` (14), `Vec4f` (15), `Vec2d` (16), `Quatf` (17), `Quatd` (18), and `TokenIndexArray` (19) typed kinds on `parity_tables.usdc`; C ABI `freeusd_read_usdc_usda_section_from_path_utf8` for embedded layer text (including zlib- and LZ4-wrapped `USDA` on `parity_embedded_scene_zlib.usdc` / `parity_embedded_scene_lz4.usdc`).
- `planned`: production USDC compression beyond the fixture wrapper, broader production value kinds, and full embedded-`USDA` scene bridge.

### Composition Semantics

- `implemented`: strongest-wins field reads, concatenated relationship lists, composed field/relationship/prim-path unions, relocated prim-path query behavior, and prefix-substituted reference/payload asset paths.
- `partial`: `subLayerOffsets` now remap composed sample times and file-backed reads; selected variants plus reference/payload/inherit/specialize arcs now affect file-backed field and prim-path queries (`parity_specializes.usda` for composed doubles through `specializes`); composed prim `kind` / `active`, `customData`, and USDA `class` / `over` specifier resolution follow references, payloads, and `inherits` / `specializes` (local layer stack still wins when authored; `parity_custom_data_refs.usda` for `customData` through references/payloads; `parity_custom_data_specializes.usda` for `customData` through specializes), composed `variantSelection` through references and payloads (`parity_variant_selection_refs.usda`, `parity_variant_selection_payloads.usda`); other metadata propagation through every arc type remains incomplete.
- `planned`: broader resolver-aware arc expansion for the remaining composed query families.

### Schema And Runtime Helpers

- `implemented`: `usdGeom::Xformable`, `usdGeom::Imageable`, `usdGeom::Boundable`, `usdUtils::FlattenStageAtTime`, and `usdUtils` engine-scene helpers for importer/editor/runtime subset inspection.
- `partial`: `usdGeom::Mesh` reads composed `points`, `extent`, `subdivisionScheme`, `faceVertexCounts`, `faceVertexIndices`, `normals`, `primvars:st`, `primvars:displayOpacity`, and `primvars:displayColor` (`parity_geom_mesh.usda`); USDA load/save accepts `texCoord2f` / `float2` and `vector3f` tuple literals.
- `partial`: flattening now preserves evaluated defaults plus composed sample times, but it does not yet reconstruct full authored layer provenance for every arc source.
- `partial`: `usdSkel::Skeleton` and `usdSkel::SkelAnimation` read joints, bind/rest matrices, and sampled TRS arrays from USDA; glTF mapping helpers build parent indices and world bind matrices; `SkelBinding` resolves `skel:skeleton` plus `primvars:skel:jointIndices` / `jointWeights`; `SkelRoot` finds skeleton and `skel:animationSource` under a scope (`parity_skel_binding.usda`); `BlendShape` / `SkelBlendShapes` / `MorphTargets` read morph offsets, remap animation weights, and apply CPU morph accumulation (`parity_skel_blend_shapes.usda`; glTF `mesh.weights` + morph target POSITION deltas); `DeformPointsWithSkeleton` performs CPU LBS from joint world matrices and inverse bind transforms (`parity_skel_skinning.usda`); `SkelRoot::FindBoundGeomPaths` lists descendants with `skel:skeleton` bindings (`parity_skel_binding.usda`); end-to-end glTF pipeline fixture (`parity_skel_gltf_pipeline.usda`: bound mesh, dual morph weights, animation-driven LBS).
- `partial`: `usdShade::Material` resolves `outputs:surface` to a shader prim; `usdShade::Shader` / `PreviewSurface` read `info:id` and common `UsdPreviewSurface` inputs (`diffuseColor`, `emissiveColor`, `metallic`, `roughness`, `opacity`) with connection following (`parity_shade_preview.usda`); texture asset paths for `diffuseColor`, `normal`, `occlusion`, `metallic`, and `roughness` resolve through one connection hop to connected `inputs:file` (`parity_shade_texture.usda`, `parity_shade_pbr_textures.usda`).
- `partial`: `usdLux::DistantLight` reads `inputs:intensity`, `inputs:color`, and `inputs:angle` at a time code (`parity_lux_distant.usda`); `usdLux::SphereLight` reads `inputs:intensity`, `inputs:color`, and `inputs:radius` (`parity_lux_sphere.usda`); `usdLux::RectLight` reads `inputs:intensity`, `inputs:color`, `inputs:width`, and `inputs:height` (`parity_lux_rect.usda`); `usdLux::DiskLight` reads `inputs:intensity`, `inputs:color`, and `inputs:radius` (`parity_lux_disk.usda`); `usdLux::CylinderLight` reads `inputs:intensity`, `inputs:color`, `inputs:length`, and `inputs:radius` (`parity_lux_cylinder.usda`); `usdLux::DomeLight` reads `inputs:intensity`, `inputs:color`, `inputs:texture:file`, and `inputs:texture:format` (`parity_lux_dome.usda`).
- `partial`: `usdPhysics::PhysicsScene` reads `physics:gravityDirection` and `physics:gravityMagnitude` at a time code (`parity_physics_scene.usda`); `usdPhysics::RigidBodyAPI` reads composed `physics:mass`, `physics:kinematicEnabled`, and detects `PhysicsRigidBodyAPI` via composed `apiSchemas` (`parity_physics_rigid_body.usda`, `parity_physics_rigid_body_inherit.usda`, `parity_physics_rigid_body_kinematic.usda`, `parity_physics_rigid_body_refs.usda`); `usdPhysics::CollisionAPI` reads composed `physics:collisionEnabled` and detects `PhysicsCollisionAPI` via composed `apiSchemas` (`parity_physics_collision.usda`, `parity_physics_collision_inherit.usda`); `usdPhysics::MassAPI` reads `physics:density` and `physics:centerOfMass` and detects `PhysicsMassAPI` via composed `apiSchemas` (`parity_physics_mass.usda`); `usdPhysics::FixedJoint` reads `physics:body0` / `physics:body1` relationship targets and `physics:jointEnabled` (`parity_physics_fixed_joint.usda`).
- `partial`: `usdVol::OpenVDBAsset` reads `filePath` and `fieldName` at a time code (`parity_vol_openvdb.usda`); `usdVol::Volume` reads prim kind and composed `field` relationship targets, resolving child `OpenVDBAsset` field prims (`parity_vol_volume.usda`).
- `token-only`: most other non-`usdGeom` / non-`usdSkel` / non-`usdShade` / non-`usdLux` / non-`usdPhysics` / non-`usdVol` schema packages remain generated token surfaces only.

### ABI And Bindings

- `implemented`: the C ABI remains the stable FFI contract for stage queries, typed field reads, crate bootstrap/TOC helpers, raw crate section payload access, and the validated `usdGeom` runtime subset (`transform`, `visibility`, `purpose`, `bounds`).
- `partial`: Python remains the broadest wrapper; the C ABI, Go, and Rust now follow the validated crate bootstrap/TOC/raw-section/table subset, composed prim `kind` / `active`, `usdGeom` imageable/boundable, `usdShade` preview-surface diffuse/texture helpers, `usdLux` DistantLight sampling, `usdVol` OpenVDBAsset reads, `usdPhysics` PhysicsScene gravity, RigidBodyAPI, CollisionAPI, and MassAPI reads, and `usdSkel` joint/skinning helpers, but broader runtime/schema helpers are still not fully mirrored outside Python/C++.
- `planned`: milestone-by-milestone expansion for more composed query families and runtime helpers once the C ABI slice is stable.

### Runtime Hardening And Deferred Stacks

- `implemented`: lightweight `plug`, `trace`, and `work` utilities exist for validation and smoke coverage.
- `intentionally deferred`: Hydra/imaging, Ndr/Sdr, and broader runtime ecosystems.

### Engine Integration Contract

- `implemented`: `docs/engine-supported-subset.md`, `docs/engine-integration.md`, clean-room/fixture/claim policy docs, `usdUtils::engineScene`, and focused engine integration tests freeze the USDA-first engine path.
- `partial`: shipping runtime remains intentionally narrow; engine snapshots list material bindings, preview-surface materials, textured preview shaders, supported `usdLux` light families, `PhysicsScene` prims, `PhysicsRigidBodyAPI`-shaped prims with composed `physics:mass` (`rigid_body_api_paths`), `PhysicsCollisionAPI`-shaped prims (`collision_api_paths`), `PhysicsFixedJoint` prims (`physics_fixed_joint_paths`), `OpenVDBAsset` prims (`open_vdb_asset_paths`), `Volume` prims (`volume_paths`), and composed prim `kind` paths; `AssessEngineRuntimeSupport` reports `uses_material_bindings`, `uses_preview_surface`, `uses_preview_surface_textures`, `uses_lux_lights`, `uses_physics_scenes`, `uses_rigid_body_api`, `uses_collision_api`, `uses_physics_fixed_joints`, `uses_open_vdb_assets`, `uses_volumes`, `uses_composed_prim_kind`, `uses_prim_active_opinions`, `uses_specializes`, `uses_kind_active_through_arcs` (including `specializes` arcs; see `parity_kind_active_specializes.usda`), and `uses_custom_data_through_arcs`; arbitrary `.usdc` scene decode and broad live-stage runtime guarantees are still out of scope.

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
