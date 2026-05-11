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
export CGO_LDFLAGS="-L/path/to/freeusd/build/src -lfreeusd_c -lfreeusd_base -lfreeusd_io -lfreeusd_usd -lfreeusd_ar -lfreeusd_pcp -lfreeusd_sdf -lfreeusd_vt -lfreeusd_tf -lfreeusd_plug -lstdc++ -lm"
cd bindings/go && go test -v ./...
```

The embedded `#cgo` lines in `freeusd.go` cover **linux** (`-lstdc++`) and **darwin** (`-lc++`). Other platforms may need a local `CGO_LDFLAGS` override. **`UsdcCrateIdentifier`** mirrors **`freeusd_usdc_crate_identifier_utf8`** (the **`PXR-USDC`** prefix). **`DetectUsdFileKindFromPath`** wraps **`freeusd_detect_usd_file_kind_from_path_utf8`** (USDA vs crate magic sniff by path). **`ReadUsdcBootstrapFromPath`** wraps **`freeusd_read_usdc_bootstrap_from_path_utf8`** (88-byte bootstrap: version + LE TOC offset). **`ReadUsdcTocFromPath`** wraps **`freeusd_read_usdc_toc_from_path_utf8`** (TOC section names + byte ranges; **`freeusd_usdc_toc_sections_free`** is called internally after copying). **`ReadFieldFloat`**, **`ReadFieldBool`**, **`ReadFieldInt64`**, **`ReadFieldString`**, **`ReadFieldVec3d`**, **`ReadFieldVec3f`**, **`ReadFieldMatrix4d`**, **`ReadFieldQuatd`**, **`ReadFieldQuatf`**, **`ReadFieldToken`**, and **`ReadFieldTokenArray`** wrap the matching **`freeusd_stage_read_field_*`** entry points. On success they return the requested value with **`rc == 0`**. On failure they return a non-zero **`rc`** plus a Go zero value / `nil`; missing prims, missing attributes, and wrong-type reads all currently map to the same C ABI failure class.

## Rust (`bindings/rust`)

The `freeusd-sys` crate links the same static libraries as the C smoke test. It exposes **`usdc_crate_identifier`**, **`detect_usd_file_kind_from_path`**, **`read_usdc_bootstrap_from_path`**, **`read_usdc_toc_from_path`**, **`Stage::open_from_root_file`**, **`Stage::prim_path_in_use`**, **`resolve_prim_specifier_kind`**, composed **layer hints** (time codes, `upAxis`, `primOrder`), **composed `relocates`**, **composed `prefixSubstitutions`**, **composed stage `customLayerData`** (string/token), and **composed prim `variantSelection` / `variantSets`** queries, plus layer attach / **`read_field_double`** / **`read_field_float`** / **`read_field_bool`** / **`read_field_int64`** / **`read_field_string`** / **`read_field_vec3d`** / **`read_field_vec3f`** / **`read_field_matrix4d`** / **`read_field_quatd`** / **`read_field_quatf`** / **`read_field_token`** / **`read_field_token_array`** smoke tests. Typed reads return **`Result<T, i32>`**: success is **`Ok(...)`**, while missing prims, missing attributes, and wrong-type reads are surfaced uniformly as **`Err(rc)`** from the C ABI. From the repo root:

```bash
cargo test --manifest-path bindings/rust/Cargo.toml
```

If the repository is not checked out at the expected path, set:

```bash
export FREEUSD_REPO_ROOT=/absolute/path/to/freeusd
cargo test --manifest-path bindings/rust/Cargo.toml
```

## Adding more bindings

New wrappers should target the **C ABI** only (no C++ ABI stability guarantees). Keep surface area small and mirror existing tests (version string + one USDA round-trip or attribute read) before exposing larger APIs. The Go package also includes **`OpenStageFromRootFile`** / **`PrimPathInUse`**, relocate, **prefixSubstitution**, **`customLayerData`**, and **prim variant** helpers, plus the **`RelocatePairSep`** constant (U+001F pair encoding for both map kinds).
