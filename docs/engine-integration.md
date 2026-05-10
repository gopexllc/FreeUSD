# Integrating FreeUSD into a C++ project (including game engines)

This document is a practical companion to [openusd-map.md](openusd-map.md) (feature coverage) and [openusd-repo-alignment.md](openusd-repo-alignment.md) (layout vs upstream OpenUSD). It focuses on **how to link** and **what to expect**, not on duplicating the full API map.

## Licensing

FreeUSD library sources are **GPL-3.0-or-later**; see the root [README](../README.md) and [LICENSE](../LICENSE). The project is aimed especially at **GPL-3 (or compatible) applications**—for example a **GPL-3 game engine**—that want a USD-shaped scene stack under the **same copyleft** as the rest of the engine, without pulling in upstream OpenUSD’s different license terms. If you **statically link** FreeUSD into a binary you **distribute**, you must comply with the GPL’s requirements for that combined work; for a GPLv3 engine, that is usually aligned with how you already ship the engine. **Proprietary** engines that cannot adopt GPLv3 for linked code still need a **separate** license or another USD implementation.

## CMake consumption

### In-tree (`add_subdirectory`)

From your project’s `CMakeLists.txt` after adding this repository (submodule, monorepo path, or `FetchContent`):

```cmake
add_subdirectory(third_party/freeusd EXCLUDE_FROM_ALL)

target_link_libraries(your_target PRIVATE freeusd::runtime)
target_include_directories(your_target PRIVATE ...) # usually unnecessary; targets propagate include/
```

`freeusd::runtime` is an **INTERFACE** aggregate defined in `src/CMakeLists.txt`: it pulls in `freeusd::base`, `tf`, `gf`, `vt`, `ar`, `sdf`, `io`, `pcp`, `usd`, `usdGeom`, `plug`, `trace`, and `work` (stubs where noted in the map). That is the closest thing to “link one CMake target for the C++ core.”

For the optional **header-only schema token bundles** as a single dependency, use:

```cmake
target_link_libraries(your_target PRIVATE freeusd::usd_schemas)
```

`usd_schemas` is an INTERFACE aggregate over **`usdGeom`**, **`usdShade`**, **`usdLux`**, **`usdPhysics`**, **`usdVol`**, **`usdSkel`**, **`usdMedia`**, **`usdRender`**, **`usdRi`**, **`usdHydra`**, **`usdMtlx`**, **`usdProc`**, **`usdSemantics`**, and **`usdUI`** (each ships `include/freeusd/<Lib>/tokens.hpp`). Token lists are regenerated from Pixar’s published **`generatedSchema.usda`** files via **`python3 scripts/gen_schema_tokens.py`** at the repository root.

The optional umbrella include **`freeusd/usd/schemaTokens.hpp`** pulls in those headers **and** **`freeusd/usd/schemaDataTokens.hpp`**, which is generated from **`pxr/usd/usd/generatedSchema.usda`** (core prim/API schema strings such as **`ModelAPI`** / **`CollectionAPI`**). That file lives next to the stage headers under **`include/freeusd/usd/`** and is distinct from the small hand-authored **`freeusd/usd/tokens.hpp`** used for Sdf-style composition field names (**`references`**, **`payload`**, …).

You can use `usd_schemas` **instead of** or **in addition to** pieces of `runtime` depending on whether you already link `runtime`—linking both is redundant but harmless for INTERFACE libs.

### Python (`_native`), schema regeneration, and pytest

- **`pyproject.toml`** targets **`requires-python >= 3.10`**. The **`project.optional-dependencies.dev`** list pulls **pytest >= 9.0.3**, which includes the fix for **CVE-2025-71176** (tmpdir / symlink handling); **`uv.lock`** in the repo root matches that resolution for **`uv`** users.
- After changing token sources, run **`python3 scripts/gen_schema_tokens.py`** from the repository root, then rebuild the **`_native`** extension (or **`pip install -e .`** so editable layouts pick up regenerated **`python/generated/*.inc`** and headers).
- **`ctest`** registers **`freeusd_pytest`** when **`pytest` ≥ 9.0.3** is available for the CMake-selected interpreter (older versions are skipped with a **configure warning**). The test uses **`PYTHONPATH`** set to the source tree so imports resolve to the in-repo **`freeusd/`** package. If CMake selects a **venv** interpreter that has **`pytest`** but **`pip install -e .`** has not been rerun since Python edits, a skbuild editable hook can still redirect **`freeusd`** to **`site-packages`**; **`tests/python/conftest.py`** removes that hook during **`pytest_configure`** so the working tree wins.
- GitHub Actions runs a dedicated **`linux-python`** job (see **`.github/workflows/ci.yml`**) that configures with **`FREEUSD_BUILD_PYTHON=ON`** (**Ninja**), restores a **`build/_deps`** cache keyed on **`CMakeLists.txt`** so **FetchContent** (**pybind11**) skips repeated git clones when the hash is unchanged, installs **pytest 9.0.3+** with **`pip`** (so runners are not stuck below the **CVE-2025-71176** fix that **`python3-pytest`** packages may omit), and runs the full **`ctest`** suite so **`freeusd_pytest`** stays green on clean Ubuntu runners.

### Build options relevant to engines

| CMake option | Default | Notes |
| --- | --- | --- |
| `FREEUSD_BUILD_PYTHON` | `ON` | Set `OFF` if you do not need the extension and want to avoid pybind11 fetch/build. |
| `FREEUSD_BUILD_TESTS` | `ON` | Set `OFF` for minimal integration builds. |
| `FREEUSD_BUILD_C_ABI` | `ON` | Set `OFF` if you only use C++ APIs and want to skip `libfreeusd_c`. |
| `FREEUSD_TEST_INSTALL_INTEGRATION` | `OFF` | When `ON` and tests are enabled, registers **`freeusd_install_integration`** CTest (full clean configure + install + `find_package` consumer; ~10s+, label **`install_integration`**). |

Example minimal configure:

```bash
cmake -S freeusd -B build/freeusd \
  -DFREEUSD_BUILD_PYTHON=OFF \
  -DFREEUSD_BUILD_TESTS=OFF \
  -DCMAKE_BUILD_TYPE=Release
cmake --build build/freeusd -j
```

### Install layout

`cmake --install` copies:

- Headers under the install prefix’s include tree (`include/freeusd/...`).
- The **`LICENSE`** file under **`${CMAKE_INSTALL_DATAROOTDIR}/doc/${PROJECT_NAME}/`** (default **`share/doc/freeusd/`** under the prefix).
- Static archives for the compiled targets (`libfreeusd_*.a`, including `libfreeusd_c.a` when `FREEUSD_BUILD_C_ABI` is on) into `${CMAKE_INSTALL_LIBDIR}`.
- **CMake package files** under `${CMAKE_INSTALL_LIBDIR}/cmake/FreeUSD/`: `FreeUSDConfig.cmake`, `FreeUSDConfigVersion.cmake`, and `FreeUsdTargets.cmake` (imported targets use the same names as in-tree, for example `freeusd::runtime`, `freeusd::usd`, `freeusd::c` when the C ABI was built).

### CPack (binary archive)

After a normal **configure** and **build**, run **`cpack`** from the **build directory**. The default generator is **TGZ** on Unix-like systems and **ZIP** on Windows, producing an archive such as **`FreeUSD-0.1.0-Linux.tar.gz`** whose root folder contains **`include/`**, **`lib/`**, **`share/doc/freeusd/LICENSE`**, **`lib/cmake/FreeUSD/`**, and **`lib/pkgconfig/freeusd.pc`**. Use **`cpack -G TGZ`** (or **`-G ZIP`**) to pick a generator explicitly.

The same **`CMAKE_INSTALL_PREFIX`** / **`.pc` prefix** caveats as for **`cmake --install`** apply: configure with the install prefix you want embedded in **`freeusd.pc`** before building and packing.

After install, a consumer project can use:

```cmake
list(APPEND CMAKE_PREFIX_PATH "/path/to/freeusd/prefix")
find_package(FreeUSD REQUIRED)
target_link_libraries(your_target PRIVATE freeusd::runtime)
```

With a **multi-config** generator (for example **Visual Studio** on Windows), configure and build a **Release** tree (or pass **`CMAKE_BUILD_TYPE=Release`** for single-config generators), and run **`ctest -C Release`** if you run tests from the build directory.

`find_package` is case-sensitive: the package name is **`FreeUSD`**. If CMake still does not locate the config file, set **`FreeUSD_DIR`** to the directory that contains **`FreeUSDConfig.cmake`** (for a typical layout that is **`${install_prefix}/lib/cmake/FreeUSD`**, or under **`lib/<triplet>/cmake/FreeUSD`** on some Linux multi-arch installs). You can still use **`add_subdirectory`** on a source checkout if you prefer not to install.

### pkg-config

A **`freeusd.pc`** file is installed under **`${CMAKE_INSTALL_LIBDIR}/pkgconfig/`**. It lists the **static** archives needed for the core stack (same ordering style as a typical `freeusd::runtime` link). On **Linux** (and **MinGW**), **`pkg-config` adds `-Wl,--start-group` / `--end-group`** so GNU `ld` can resolve circular references between the `.a` files; on **macOS** those flags are omitted because Apple’s linker does not support them. When the C ABI was built, **`-lfreeusd_c`** is included.

```bash
export PKG_CONFIG_PATH="/path/to/freeusd/prefix/lib/pkgconfig"
g++ -std=c++17 main.cpp $(pkg-config --cflags --libs freeusd) -o app
```

The `.pc` file’s **`prefix=`** is taken from **`CMAKE_INSTALL_PREFIX` at configure time**. If you install with `cmake --install … --prefix /other` **without** that matching your configure-time prefix, regenerate the build with **`-DCMAKE_INSTALL_PREFIX=/other`** before installing so `prefix`, `libdir`, and `includedir` stay consistent (the same caveat applies to many CMake-generated pkg-config files).

### C++ vs C ABI

- **C++**: include headers under `freeusd/...` (for example `freeusd/usd/stage.hpp`) and link `freeusd::runtime` or finer-grained `freeusd::usd` etc.
- **C ABI** (stable FFI boundary): link `freeusd::c`, include [`include/freeusd/c/freeusd.h`](../include/freeusd/c/freeusd.h). Useful for engine subsystems written in C, another language, or a DLL boundary you want to keep narrow. USDC helpers include **`freeusd_usdc_crate_identifier_utf8`**, **`freeusd_detect_usd_file_kind_from_path_utf8`** (ASCII vs crate magic by path prefix only), **`freeusd_read_usdc_bootstrap_from_path_utf8`** (**`FreeusdUsdcBootstrap`**: version bytes + little-endian TOC offset), and **`freeusd_read_usdc_toc_from_path_utf8`** (**`FreeusdUsdcTocSection`** + **`freeusd_usdc_toc_sections_free`**).

## Threading

The C header documents that **`FreeusdLayer` / `FreeusdStage` must not be used from multiple threads concurrently** unless you add your own synchronization, and that `freeusd_last_error_message` is **thread-local**. Treat the C++ `Stage`, layers, and related mutable objects the same way unless a future release documents stronger guarantees: **one thread at a time per stage/layer graph**, or external locking.

## Formats and runtime expectations

- **USDA (ASCII)**: supported for load/save via `freeusd::io::usda` and stage open-from-file paths; this is the practical interchange format today.
- **USDC (crate)**: **full decode is not implemented**, but **`PXR-USDC`** prefix sniffing, **USDA vs crate** path classification, a **bootstrap** read, and a **TOC section list** read (little-endian count + fixed-size records; no token/path/string tables) exist in C++ (`ReadUsdCrateBootstrapFromPath`, **`ReadUsdCrateTocFromPath`**), Python, the C ABI (**`freeusd_read_usdc_toc_from_path_utf8`** + **`freeusd_usdc_toc_sections_free`**), and Go / Rust. **`freeusd_detect_usd_file_kind_from_path_utf8`** remains **sniff-only** (no bootstrap). Pipelines that need **full** binary crate payloads still need another reader or an export path to USDA.
- **Hydra / UsdImaging**: **not present**. There is no built-in path from FreeUSD to GPU scene delegates like upstream USD’s imaging stack.

## Fit checklist before shipping

1. **License**: GPLv3 engines align with FreeUSD’s **GPL-3.0-or-later**; proprietary stacks need a different USD path or a separate license.
2. **Asset format**: USDA-only vs need for `.usdc`.
3. **Feature subset**: compare your required composition and schema behavior with [openusd-map.md](openusd-map.md); FreeUSD is intentionally smaller and **not** drop-in API-compatible with Pixar OpenUSD.
4. **Threading**: serialize access or clone per thread if you load in workers.
5. **Link model**: default build is **static** `.a` libraries; `BUILD_SHARED_LIBS` affects `FREEUSD_SHARED` define for any future shared usage—confirm what your platform ships.

For a high-level “what exists vs upstream,” keep [openusd-map.md](openusd-map.md) as the source of truth.
