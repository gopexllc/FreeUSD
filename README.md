# FreeUSD

A **GPL-3.0-or-later** licensed, independent implementation of the OpenUSD (Universal Scene Description) file formats and core data model.

It is intended for **GPL-3-compatible** stacks (for example a **GPL-3 game engine**) that want USD-style authoring and composition under the **same license family** as the engine, rather than depending on upstream OpenUSD’s license for that layer.

---

FreeUSD is an independent project and is not endorsed by or affiliated with Pixar, the Alliance for OpenUSD, or the official OpenUSD project. It aims for compliance with the published OpenUSD Core Specification.

## Licensing

All **FreeUSD** library and tool sources in this repository are licensed under the **GNU General Public License v3.0 or later**; see [`LICENSE`](LICENSE).

Implementation is **clean-room** relative to Pixar’s OpenUSD sources: **do not copy upstream OpenUSD/AOUSD code** into this repository; only independent implementations. See [docs/openusd-repo-alignment.md](docs/openusd-repo-alignment.md). Compatibility targets **published formats and behavior**, not upstream source.

**Third-party components** used at build or test time remain under their own licenses (for example **pybind11**, fetched by CMake when the Python module is enabled, is BSD-style and commonly paired with GPLv3 software). You are responsible for satisfying the license terms of any dependency you ship alongside FreeUSD.

## Layout

C++ libraries follow OpenUSD-style layering: `tf`, `gf`, `vt`, `ar`, `sdf`, `pcp`, `usd`, `usdGeom`, plus `plug`, `trace`, and `work` stubs.

| Topic | Doc |
| --- | --- |
| Feature map vs OpenUSD | [docs/openusd-map.md](docs/openusd-map.md) |
| Parity baseline and fixture corpus | [docs/openusd-parity-matrix.md](docs/openusd-parity-matrix.md) |
| Repo / CMake / out-of-scope | [docs/openusd-repo-alignment.md](docs/openusd-repo-alignment.md) |
| Embedding, `find_package`, pkg-config | [docs/engine-integration.md](docs/engine-integration.md) |
| Frozen engine contract | [docs/engine-supported-subset.md](docs/engine-supported-subset.md) |
| Clean-room / fixture / claim rules | [CONTRIBUTING.md](CONTRIBUTING.md) |
| Cursor agent onboarding / SDK CLI | [AGENTS.md](AGENTS.md), [tools/agent/](tools/agent/README.md) |

**USDA:** minimal ASCII load/save (`freeusd::io::usda`, Python `freeusd.io`), including `attr.timeSamples = { t: v, … }` blocks.

**`.usdc` (crate):** Full binary decode is still **partial**. What exists today: `PXR-USDC` sniffing, USDA vs crate path classification, a fixed **bootstrap** read (version bytes + little-endian TOC offset), a **TOC section table** read (LE section count + 32-byte name/start/size rows), raw **section payload** reads, validated **`TOKENS` / `STRINGS` / `PATHS`** table decode, and a narrow embedded-`USDA` stage-open fallback for controlled fixtures/pipelines. Surfaces in C++ (`freeusd::usd::crate`), Python (`freeusd.usd.crate`), the **C ABI**, and thin **Go** / **Rust** bindings under [`bindings/`](bindings/README.md).

## Building

**C++ (library, tests, optional Python extension in the build tree)**

```bash
cmake -S . -B build -DFREEUSD_BUILD_PYTHON=ON -DFREEUSD_BUILD_TESTS=ON
cmake --build build -j
ctest --test-dir build --output-on-failure
```

**Continuous integration** ([`.github/workflows/ci.yml`](.github/workflows/ci.yml), [troubleshooting](docs/github-actions.md)):

If Actions jobs fail instantly with a billing lock message, fix org billing or run [`scripts/run_ci_locally.sh`](scripts/run_ci_locally.sh) locally.

- **Linux / macOS / Windows** (VS 2022 Release): C++ tests, Python **off**, `FREEUSD_TEST_INSTALL_INTEGRATION=ON` for `find_package`.
- **`tree-hygiene`:** fails if any `build-*` path is tracked in git and verifies the engine contract / clean-room docs remain present.
- **`linux-python`:** Ninja build with Python **on**, **pytest ≥ 9.0.3** via pip (CVE-2025-71176), `ctest` including **`freeusd_pytest`**.
- **`linux-bindings`:** C++ without Python; **`cargo test`** (`bindings/rust`) and **`go test`** (`bindings/go`, CGO).

Concurrency cancels superseded runs on the same branch. [`.github/dependabot.yml`](.github/dependabot.yml) proposes updates (Actions monthly, **uv** weekly, **Cargo** under `bindings/rust` monthly). Manual runs: **Actions → CI → Run workflow**.

**Install / packaging:** set `-DCMAKE_INSTALL_PREFIX=` to your target prefix, then `cmake --install build`. From the build tree, **`cpack`** can produce **TGZ** or **ZIP** archives with headers, static libs, CMake config, and `freeusd.pc`. Details: [docs/engine-integration.md](docs/engine-integration.md).

**Python tests (`freeusd_pytest`):** Registered when both tests and Python are enabled **and** `pytest` ≥ 9.0.3 is found at configure time; otherwise CMake warns and skips (upgrade pytest, then re-run `cmake`). Build target **`_native`** first. **`ctest`** sets **`PYTHONPATH`** to the repo root so the in-tree **`freeusd/`** package is used. For ad hoc **`pytest`**, use the same **`PYTHONPATH`** or **`pip install -e .`**; after edits under `freeusd/`, re-install editable so **site-packages** copies stay fresh.

**Schema token headers** (`include/freeusd/<UsdLib>/tokens.hpp`, `include/freeusd/usd/schemaDataTokens.hpp`): regenerate with `python3 scripts/gen_schema_tokens.py`, then rebuild **`_native`**. **`tests/python/conftest.py`** avoids the skbuild editable hook during pytest so **`ctest`** and **`.venv`** runs both load the working-tree package.

**C ABI** (on by default: `-DFREEUSD_BUILD_C_ABI=ON`): link **`freeusd_c`** and include [`include/freeusd/c/freeusd.h`](include/freeusd/c/freeusd.h). Stable entry points cover in-memory and on-disk **USDA** layers, **layer stacks**, **composed stages** (prim paths, fields, time samples, composition arcs, relocates, prefix substitutions, variants, layer hints, custom layer data, specifier kind, and related queries), the validated `usdGeom` runtime subset (transform, visibility, purpose, bounds), plus **USDC** helpers (crate id string, path kind sniff, bootstrap struct, TOC sections, raw section bytes, structured table reads, and the narrow embedded-`USDA` stage-open fallback). Full symbol list, ownership rules, and thread-local errors are documented in the header; prefer that over duplicating API names here.

The extension is built as **`_native*.so`** under `build/`; CMake can copy it beside `freeusd/` (gitignored `*.so`) so `import freeusd._native` works with **`PYTHONPATH`** at the repo root.

## Python package (development)

Requires **Python 3.10+** (`requires-python` in [`pyproject.toml`](pyproject.toml)). The **`dev`** extra pins **pytest 9.0.3+** (CVE-2025-71176), aligned with **`uv.lock`** for local tooling.

```bash
python3 -m venv .venv
source .venv/bin/activate   # Windows: .venv\Scripts\activate
pip install -e ".[dev]"
pytest tests/python -q
```

## Other languages

Thin **Go** and **Rust** wrappers over the C ABI live under [`bindings/`](bindings/README.md) (`bindings/go`, `bindings/rust/freeusd-sys`). Build the C++ project first, then follow that README for **`go test`** / **`cargo test`**. Other languages can use the same pattern (FFI to `libfreeusd_c` plus the static archives listed there).
