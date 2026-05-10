# FreeUSD
A **GPL-3.0-or-later** licensed, independent implementation of the OpenUSD (Universal Scene Description) file formats and core data model.

It is intended for **GPL-3-compatible** stacks—for example a **GPL-3 game engine**—that want USD-style authoring and composition under the **same license family** as the engine, rather than depending on upstream OpenUSD’s license for that layer.

---

FreeUSD is an independent project and is not endorsed by or affiliated with Pixar, the Alliance for OpenUSD, or the official OpenUSD project. It aims for compliance with the published OpenUSD Core Specification.

## Licensing

All **FreeUSD** library and tool sources in this repository are licensed under the **GNU General Public License v3.0 or later**; see [`LICENSE`](LICENSE). That grants you the usual copyleft freedoms and obligations when you distribute modified or combined works.

Implementation is **clean-room** relative to Pixar’s OpenUSD sources: this tree does not incorporate upstream OpenUSD/AOUSD code; see [docs/openusd-repo-alignment.md](docs/openusd-repo-alignment.md). Compatibility is aimed at **published formats and behavior**, not at copying upstream implementation.

**Third-party components** used at build or test time remain under their own licenses (for example **pybind11**, fetched by CMake when the Python module is enabled, is under a BSD-style license that is widely used together with GPLv3 software). You are responsible for satisfying the license terms of any dependency you ship alongside FreeUSD.

## Layout

C++ libraries follow OpenUSD-style layering (`tf`, `gf`, `vt`, `ar`, `sdf`, `pcp`, `usd`, `usdGeom`, plus `plug` / `trace` / `work` stubs). See [docs/openusd-map.md](docs/openusd-map.md) for a side-by-side map and [docs/openusd-repo-alignment.md](docs/openusd-repo-alignment.md) for how this tree maps to a full **OpenUSD** checkout (names, CMake targets, and out-of-scope areas)—without copying upstream code. For **embedding in another CMake project** (game engines, tools), see [docs/engine-integration.md](docs/engine-integration.md). **USDA:** minimal ASCII load/save (`freeusd::io::usda` / `freeusd.io`), including `attr.timeSamples = { t: v, ... }` blocks. **`.usdc`:** **full binary decode is not implemented**; **`PXR-USDC`** magic and **USDA vs crate** path sniffing exist (C++ / C ABI / Python / Go / Rust; see **`freeusd.usd.crate`**).

## Building

**C++ (library, tests, extension in the build tree)**

```bash
cmake -S . -B build -DFREEUSD_BUILD_PYTHON=ON -DFREEUSD_BUILD_TESTS=ON
cmake --build build -j
ctest --test-dir build --output-on-failure
```

Continuous integration (GitHub Actions) runs **Linux**, **macOS**, and **Windows** (Visual Studio 2022, **Release**) with **Python disabled**, **tests on**, and **`FREEUSD_TEST_INSTALL_INTEGRATION=ON`** so **`find_package`** is exercised; see [`.github/workflows/ci.yml`](.github/workflows/ci.yml). The workflow uses **concurrency** to cancel in-progress runs when the same branch is updated; **[`.github/dependabot.yml`](.github/dependabot.yml)** proposes version updates (**GitHub Actions** monthly, **uv** weekly, **Cargo** under **`bindings/rust`** monthly). A separate **Linux** job (**`linux-python`**) builds with **`FREEUSD_BUILD_PYTHON=ON`** (**Ninja** generator), caches **`build/_deps`** so **FetchContent** (**pybind11**) reconfigures faster across runs, installs **pytest 9.0.3+** via **pip** (Ubuntu’s **`python3-pytest`** package can lag below the CVE-2025-71176 fix), and runs **`ctest`** including **`freeusd_pytest`**. Another **Linux** job (**`linux-bindings`**) builds the C++ stack without Python and runs **`cargo test`** on **`bindings/rust/freeusd-sys`** and **`go test`** under **`bindings/go`** (CGO). Workflows can also be started manually (**Actions → CI → Run workflow**).

To install headers, static libraries, **`find_package(FreeUSD)`**, and **`pkg-config`** (`freeusd.pc`): configure with **`-DCMAKE_INSTALL_PREFIX=`** matching the tree you will install into, then `cmake --install build` (or `--prefix` same as that). From the build directory, **`cpack`** produces a **TGZ** or **ZIP** binary archive. See [docs/engine-integration.md](docs/engine-integration.md).

If Python and tests are both enabled and **`pytest`** is importable at configure time, **`ctest`** also registers **`freeusd_pytest`** (label **`python`**). Re-run **`cmake -S . -B build`** after installing pytest so the test appears. Build **`_native`** before pytest so `freeusd/_native*.so` exists beside the package. **`ctest`** sets **`PYTHONPATH`** to the repository root so imports resolve to the in-tree **`freeusd/`** package; for **`pytest`** run by hand, use the same **`PYTHONPATH`** or rely on **`pip install -e .`**. Editable (**skbuild**) installs copy the Python tree into **`site-packages`**; after pulling changes under **`freeusd/`**, run **`pip install -e .`** again so that copy stays in sync.

**Schema token headers** (OpenUSD-shaped **`TfToken`** string constants under **`include/freeusd/<UsdLib>/tokens.hpp`**, plus **`include/freeusd/usd/schemaDataTokens.hpp`** from **`pxr/usd/usd/generatedSchema.usda`**) are generated with **`python3 scripts/gen_schema_tokens.py`**, then rebuild **`_native`** so the Python extension stays aligned. **`tests/python/conftest.py`** drops the skbuild editable import hook during **`pytest`** so **`ctest`**’s **`PYTHONPATH`** to the repo consistently loads the working-tree **`freeusd/`** package (even when **`pytest`** runs from a **`.venv`**).

**C ABI** (optional; `-DFREEUSD_BUILD_C_ABI=ON` by default): link `freeusd_c` and include [`include/freeusd/c/freeusd.h`](include/freeusd/c/freeusd.h). **`freeusd_usdc_crate_identifier_utf8`** returns the **`PXR-USDC`** crate file prefix (static storage; do not free). **`freeusd_detect_usd_file_kind_from_path_utf8`** classifies a file path by prefix (**`#usda`** vs crate magic; no full USDC decode) into **`FreeusdUsdFileKind`**; optional **`out_detail_utf8`** uses **`freeusd_string_free`**. Opaque `FreeusdLayer`, `FreeusdLayerStack`, `FreeusdStage`; USDA load/save; layer **identifier**, **documentation**, **defaultPrim**, **subLayers** list, **sorted prim path** list; `freeusd_stage_attach_root_layer` / `freeusd_stage_attach_layer_stack`; compose layer count; **composed prim path** union; root **defaultPrim**; child path listing; relationship target listing (`freeusd_stage_list_relationship_targets` + `freeusd_stage_has_relationship`); **composed composition arcs** (`freeusd_stage_list_prim_references`, `freeusd_stage_list_prim_inherits`, `freeusd_stage_list_prim_specializes`, `freeusd_stage_list_prim_payloads`, and matching `freeusd_stage_has_prim_*`); **`freeusd_stage_open_from_root_file_utf8`** (USDA on disk + optional `subLayers` stacking: `0` / `1` / `2` = none / shallow / depth-first); composed layer **`relocates`** (`freeusd_stage_relocate_source_in_any_layer`, `freeusd_stage_get_composed_relocate_target_utf8`, `freeusd_stage_list_composed_relocate_pairs_utf8` with U+001F-separated paths per pair) and **`prefixSubstitutions`** (`freeusd_stage_prefix_substitution_key_in_any_layer`, `freeusd_stage_get_composed_prefix_substitution_utf8`, `freeusd_stage_list_composed_prefix_substitution_pairs_utf8`, same U+001F pair encoding); **composed** attribute **field-name** and **relationship-name** unions across the stack; `freeusd_stage_has_field_opinion`; **attribute connections** (`freeusd_stage_has_attribute_connection`, `freeusd_stage_get_attribute_connection_target`); `freeusd_stage_read_field_double` / `_bool` / `_int64` / `_string` / `_vec3d`; **composed time-sample** times (`freeusd_stage_list_field_sample_times` + `freeusd_double_array_free`); composed `active` + **`active` opinion** query, composed prim **`def` / `class` / `over`** (`freeusd_stage_resolve_prim_specifier_kind`, `FreeusdPrimSpecifierKind`), `kind`, `customData` (string/token as UTF-8), **customData key** presence and **sorted key list**; composed stage **`customLayerData`** (`freeusd_stage_get_composed_custom_layer_data_utf8` for string/token values, `freeusd_stage_custom_layer_data_key_in_any_layer`, `freeusd_stage_list_composed_custom_layer_data_keys`); composed prim **`variantSelection`** / **`variantSets`** (`freeusd_stage_get_composed_prim_variant_selection_utf8`, `freeusd_stage_prim_variant_selection_set_in_any_layer`, `freeusd_stage_list_composed_prim_variant_selection_sets_utf8`, `freeusd_stage_list_composed_prim_variant_set_names_utf8`, `freeusd_stage_prim_variant_set_declared_in_any_layer`, `freeusd_stage_list_composed_prim_variant_names_utf8`); **pseudo-root path** string; composed layer **hints** (`freeusd_stage_get_start_time_code`, `_get_end_time_code`, `_get_time_codes_per_second`, `_get_frames_per_second`, `_get_frame_precision`, `_get_meters_per_unit` each with `out_has`; `freeusd_stage_get_up_axis_utf8`; `freeusd_stage_list_prim_order_paths_utf8`). Returned `char*` strings use `freeusd_string_free`; path arrays from `freeusd_stage_list_child_paths` (and relationship listing, composition-arc lists, customData / variant string lists) use `freeusd_path_list_free`. Last error text is thread-local.

The native Python module is built as `_native*.so` under `build/`; CMake also copies it into `freeusd/` (gitignored `*.so`) so `import freeusd._native` works when the repo root is on `PYTHONPATH`. For wheels and editable installs, use `pip install -e ".[dev]"` as below.

**Python package (recommended for development)**

The wheel / editable metadata require **Python 3.10+** (see `requires-python` in [`pyproject.toml`](pyproject.toml)). The **`dev`** extra pins **pytest 9.0.3+**, which fixes **CVE-2025-71176** (tmpdir handling) and matches what **`uv.lock`** resolves for local tooling.

```bash
python3 -m venv .venv
source .venv/bin/activate   # Windows: .venv\Scripts\activate
pip install -e ".[dev]"
pytest tests/python -q
```

## Other languages

Thin **Go** and **Rust** bindings over the stable C ABI live under [`bindings/`](bindings/README.md) (`bindings/go`, `bindings/rust/freeusd-sys`). Build the C++ project first, then follow that README for `go test` / `cargo test`. Additional languages can follow the same pattern (FFI to `libfreeusd_c` + the static dependency chain listed there).
