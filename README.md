# FreeUSD
A GPL-3.0 licensed, independent implementation of the OpenUSD (Universal Scene Description) file formats and core data model.

---

FreeUSD is an independent project and is not endorsed by or affiliated with Pixar, the Alliance for OpenUSD, or the official OpenUSD project. It aims for compliance with the published OpenUSD Core Specification.

## Layout

C++ libraries follow OpenUSD-style layering (`tf`, `gf`, `vt`, `ar`, `sdf`, `pcp`, `usd`, `usdGeom`, plus `plug` / `trace` / `work` stubs). See [docs/openusd-map.md](docs/openusd-map.md) for a side-by-side map and [docs/openusd-repo-alignment.md](docs/openusd-repo-alignment.md) for how this tree maps to a full **OpenUSD** checkout (names, CMake targets, and out-of-scope areas)—without copying upstream code. **USDA:** minimal ASCII load/save (`freeusd::io::usda` / `freeusd.io`), including `attr.timeSamples = { t: v, ... }` blocks. **`.usdc`:** not implemented yet (header placeholder only).

## Building

**C++ (library, tests, extension in the build tree)**

```bash
cmake -S . -B build -DFREEUSD_BUILD_PYTHON=ON -DFREEUSD_BUILD_TESTS=ON
cmake --build build -j
ctest --test-dir build --output-on-failure
```

If Python and tests are both enabled and **`pytest`** is importable at configure time, **`ctest`** also registers **`freeusd_pytest`** (label **`python`**). Re-run **`cmake -S . -B build`** after installing pytest so the test appears. Build **`_native`** before pytest so `freeusd/_native*.so` exists beside the package.

**C ABI** (optional; `-DFREEUSD_BUILD_C_ABI=ON` by default): link `freeusd_c` and include [`include/freeusd/c/freeusd.h`](include/freeusd/c/freeusd.h). Opaque `FreeusdLayer`, `FreeusdLayerStack`, `FreeusdStage`; USDA load/save; layer **identifier**, **documentation**, **defaultPrim**, **subLayers** list, **sorted prim path** list; `freeusd_stage_attach_root_layer` / `freeusd_stage_attach_layer_stack`; compose layer count; **composed prim path** union; root **defaultPrim**; child path listing; relationship target listing (`freeusd_stage_list_relationship_targets` + `freeusd_stage_has_relationship`); **composed composition arcs** (`freeusd_stage_list_prim_references`, `freeusd_stage_list_prim_inherits`, `freeusd_stage_list_prim_specializes`, `freeusd_stage_list_prim_payloads`, and matching `freeusd_stage_has_prim_*`); **composed** attribute **field-name** and **relationship-name** unions across the stack; `freeusd_stage_has_field_opinion`; **attribute connections** (`freeusd_stage_has_attribute_connection`, `freeusd_stage_get_attribute_connection_target`); `freeusd_stage_read_field_double` / `_bool` / `_int64` / `_string` / `_vec3d`; **composed time-sample** times (`freeusd_stage_list_field_sample_times` + `freeusd_double_array_free`); composed `active` + **`active` opinion** query, `kind`, `customData` (string/token as UTF-8), **customData key** presence and **sorted key list**; **pseudo-root path** string. Returned `char*` strings use `freeusd_string_free`; path arrays from `freeusd_stage_list_child_paths` (and relationship listing, composition-arc lists, and customData key lists) use `freeusd_path_list_free`. Last error text is thread-local.

The native Python module is built as `_native*.so` under `build/`; CMake also copies it into `freeusd/` (gitignored `*.so`) so `import freeusd._native` works when the repo root is on `PYTHONPATH`. For wheels and editable installs, use `pip install -e ".[dev]"` as below.

**Python package (recommended for development)**

```bash
python3 -m venv .venv
source .venv/bin/activate   # Windows: .venv\Scripts\activate
pip install -e ".[dev]"
pytest tests/python -q
```

## Other languages

Thin **Go** and **Rust** bindings over the stable C ABI live under [`bindings/`](bindings/README.md) (`bindings/go`, `bindings/rust/freeusd-sys`). Build the C++ project first, then follow that README for `go test` / `cargo test`. Additional languages can follow the same pattern (FFI to `libfreeusd_c` + the static dependency chain listed there).
