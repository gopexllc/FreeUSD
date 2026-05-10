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

The embedded `#cgo` lines in `freeusd.go` cover **linux** (`-lstdc++`) and **darwin** (`-lc++`). Other platforms may need a local `CGO_LDFLAGS` override. **`UsdcCrateIdentifier`** mirrors **`freeusd_usdc_crate_identifier_utf8`** (the **`PXR-USDC`** prefix). **`DetectUsdFileKindFromPath`** wraps **`freeusd_detect_usd_file_kind_from_path_utf8`** (USDA vs crate magic sniff by path). **Bootstrap** (`ReadUsdCrateBootstrapFromPath`) is C++/Python today; extend the C ABI if you need it from Go/Rust.

## Rust (`bindings/rust`)

The `freeusd-sys` crate links the same static libraries as the C smoke test. It exposes **`usdc_crate_identifier`**, **`detect_usd_file_kind_from_path`**, **`Stage::open_from_root_file`**, **`Stage::prim_path_in_use`**, **`resolve_prim_specifier_kind`**, composed **layer hints** (time codes, `upAxis`, `primOrder`), **composed `relocates`**, **composed `prefixSubstitutions`**, **composed stage `customLayerData`** (string/token), and **composed prim `variantSelection` / `variantSets`** queries, plus layer attach / `read_field_double` smoke tests. From the repo root:

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
