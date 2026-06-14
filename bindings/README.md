# FreeUSD language bindings

Beyond **C++** and the stable **C ABI** ([`include/freeusd/c/freeusd.h`](../include/freeusd/c/freeusd.h)), this directory holds thin wrappers for other languages. They all rely on the same static archives produced by the main CMake build (`build/src/libfreeusd_*.a`).

## Prerequisites

From the repository root:

```bash
cmake -S . -B build -DFREEUSD_BUILD_C_ABI=ON -DFREEUSD_BUILD_TESTS=ON
cmake --build build -j
```

## Python (reference)

The supported first-class binding is the **pybind11** module built by CMake (`_native`). See the root [README](../README.md#python-package-recommended-for-development).

## Go (`bindings/go`)

Requires **Go 1.21+** and **CGO** with a C++ toolchain.

Default `cgo` flags assume the repo layout `../../include` and `../../build/src` relative to this package (i.e. build directory is `freeusd/build`). Override if needed:

```bash
export CGO_CFLAGS="-I/path/to/freeusd/include"
export CGO_LDFLAGS="-L/path/to/freeusd/build/src -lfreeusd_c -lfreeusd_base -lfreeusd_usdUtils -lfreeusd_usdGeom -lfreeusd_usd -lfreeusd_ar -lfreeusd_io -lfreeusd_pcp -lfreeusd_sdf -lfreeusd_vt -lfreeusd_tf -lfreeusd_gf -lfreeusd_plug -lstdc++ -lm"
cd bindings/go && go test -v ./...
```

The embedded `#cgo` lines in `freeusd.go` cover **linux** (`-lstdc++`) and **darwin** (`-lc++`). Other platforms may need a local `CGO_LDFLAGS` override. **`UsdcCrateIdentifier`** mirrors **`freeusd_usdc_crate_identifier_utf8`** (the **`PXR-USDC`** prefix). **`DetectUsdFileKindFromPath`** wraps **`freeusd_detect_usd_file_kind_from_path_utf8`** (USDA vs crate magic sniff by path). **`ReadUsdcBootstrapFromPath`** wraps **`freeusd_read_usdc_bootstrap_from_path_utf8`** (88-byte bootstrap: version + LE TOC offset). **`ReadUsdcTocFromPath`** wraps **`freeusd_read_usdc_toc_from_path_utf8`** (TOC section names + byte ranges; **`freeusd_usdc_toc_sections_free`** is called internally after copying). **`ReadUsdcSectionBytesFromPath`** wraps **`freeusd_read_usdc_section_bytes_from_path_utf8`** (raw payload bytes for one named TOC section; **`freeusd_bytes_free`** is called internally after copying). The validated scene-open path is still **USDA-first**; the C ABI also exposes a narrow embedded-`USDA` crate fallback through **`freeusd_stage_open_from_root_file_utf8`**, but not general `.usdc` scene decode. **`ReadFieldFloat`**, **`ReadFieldBool`**, **`ReadFieldInt64`**, **`ReadFieldString`**, **`ReadFieldVec3d`**, **`ReadFieldVec3f`**, **`ReadFieldMatrix4d`**, **`ReadFieldQuatd`**, **`ReadFieldQuatf`**, **`ReadFieldToken`**, and **`ReadFieldTokenArray`** wrap the matching **`freeusd_stage_read_field_*`** entry points. **`ResolvePrimKind`** / **`ResolvePrimActive`** mirror composed prim kind and active C ABI queries, and **`ReadMaterialSurfaceShaderPath`** / **`ReadPreviewSurfaceDiffuseColor`** / **`ReadPreviewSurfaceDiffuseTextureAssetPath`** mirror the validated `usdShade` preview-surface slice. On success they return the requested value with **`rc == 0`**. On failure they return a non-zero **`rc`** plus a Go zero value / `nil`; missing prims, missing attributes, and wrong-type reads all currently map to the same C ABI failure class.

## Rust (`bindings/rust`)

The `freeusd-sys` crate links the same static libraries as the C smoke test. It exposes **`usdc_crate_identifier`**, **`detect_usd_file_kind_from_path`**, **`read_usdc_bootstrap_from_path`**, **`read_usdc_toc_from_path`**, **`read_usdc_section_bytes_from_path`**, **`Stage::open_from_root_file`**, **`Stage::prim_path_in_use`**, **`resolve_prim_specifier_kind`**, composed **prim kind / active**, composed **layer hints** (time codes, `upAxis`, `primOrder`), **composed `relocates`**, **composed `prefixSubstitutions`**, **composed stage `customLayerData`** (string/token), and **composed prim `variantSelection` / `variantSets`** queries, plus layer attach / **`read_field_double`** / **`read_field_float`** / **`read_field_bool`** / **`read_field_int64`** / **`read_field_string`** / **`read_field_vec3d`** / **`read_field_vec3f`** / **`read_field_matrix4d`** / **`read_field_quatd`** / **`read_field_quatf`** / **`read_field_token`** / **`read_field_token_array`** / **`usdShade` preview-surface** smoke tests. `Stage::open_from_root_file` follows the same USDA-first policy as the C ABI, including the narrow embedded-`USDA` crate fallback but not arbitrary binary scene decode. Typed reads return **`Result<T, i32>`**: success is **`Ok(...)`**, while missing prims, missing attributes, and wrong-type reads are surfaced uniformly as **`Err(rc)`** from the C ABI. From the repo root:

```bash
cargo test --manifest-path bindings/rust/Cargo.toml
```

If the repository is not checked out at the expected path, set:

```bash
export FREEUSD_REPO_ROOT=/absolute/path/to/freeusd
cargo test --manifest-path bindings/rust/Cargo.toml
```

## Cross-language coverage (composition + reads)

Shared fixture: **`tests/fixtures/usd_cross_language.usda`**. Tests: **`tests/c_usd_cross_language.c`**, **`tests/python/test_usd_cross_language.py`**, **`bindings/go/freeusd_test.go`** (`TestCrossLanguageFieldReadContract`), **`bindings/rust`** (`cross_language_field_read_contract`).

Skel fixture: **`tests/fixtures/parity_skel_skinning.usda`**. Tests: **`tests/c_usd_skel.c`**, **`tests/usd_skel_smoke_test.cpp`**, **`bindings/go/freeusd_test.go`** (`TestSkelCrossLanguageContract`), **`bindings/rust`** (`skel_cross_language_contract`).

| Capability | C ABI | C++ / Python | Go | Rust |
|------------|:-----:|:------------:|:--:|:----:|
| Typed field reads (double/float/bool/int/string/vec/matrix/quat/token) | yes | yes | yes | yes |
| Composed field sample times | yes | yes | yes | yes |
| Field opinion / connection queries | yes | yes | yes | yes |
| Prim arc lists (references, payloads, inherits, specializes) | yes | yes | yes | yes |
| Composed prim customData (string/token + int64) | yes | yes (typed `Value`) | yes | yes |
| Composed prim customData key list / layer presence | yes | yes | yes | yes |
| Composed prim kind / active | yes | yes | yes | yes |
| `usdGeom` imageable/boundable (C ABI) | yes | yes | yes | yes |
| `usdShade` preview-surface diffuse / texture path (C ABI) | yes | yes | yes | yes |
| `usdLux` DistantLight sample (C ABI) | yes | yes | yes | yes |
| `usdVol` OpenVDBAsset file / field (C ABI) | yes | yes | yes | yes |
| `usdPhysics` PhysicsScene gravity (C ABI) | yes | yes | yes | yes |
| `usdPhysics` RigidBodyAPI mass / kinematic (C ABI) | yes | yes | yes | yes |
| `usdPhysics` CollisionAPI enabled (C ABI) | yes | yes | yes | yes |
| `usdPhysics` MassAPI density / center of mass (C ABI) | yes | yes | yes | yes |
| `usdSkel` joint names / skinning palette / CPU deform | yes | yes | yes | yes |
| `usdUtils` engine runtime assess (skel flags) | yes | yes | yes | yes |
| `usdUtils` flatten | C++ only | yes | n/a | n/a |

## Adding more bindings

New wrappers should target the **C ABI** only (no C++ ABI stability guarantees). Keep surface area small and mirror existing tests (version string + one USDA round-trip or attribute read) before exposing larger APIs. The Go package also includes **`OpenStageFromRootFile`** / **`PrimPathInUse`**, relocate, **prefixSubstitution**, **`customLayerData`**, **raw USDC section bytes**, **prim variant** helpers, composition helpers (**`ListFieldSampleTimes`**, **`HasFieldOpinion`**, prim arc list/has, composed prim **`customData`** / **kind** / **active**), **`usdShade`** preview-surface helpers, **`usdLux`** DistantLight sampling, **`usdVol`** OpenVDBAsset reads, **`usdPhysics`** PhysicsScene / RigidBodyAPI / CollisionAPI / MassAPI reads, and the validated **`usdGeom`** engine subset on **`Stage`**: **`ComputeLocalTransformMatrix4d`**, **`ComputeLocalToWorldTransformMatrix4d`**, **`ComputeImageableVisibility`**, **`ComputeImageablePurpose`**, **`ComputeBoundableLocalBounds`**, **`ComputeBoundableWorldBounds`** (mirroring the C ABI). Rust exposes the same surface on **`Stage`**. Cross-language checks use **`tests/fixtures/parity_imageable.usda`** (aligned with **`freeusd_c_engine_integration`**) and **`usd_cross_language.usda`** for composition parity.
