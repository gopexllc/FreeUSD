# Aligning FreeUSD with the OpenUSD repository (clean-room)

This project tracks the **shape** of the [OpenUSD](https://github.com/PixarAnimationStudios/OpenUSD) monorepo—library boundaries, public concepts, and where extensions live—so contributors and integrators can navigate FreeUSD with familiar mental models. **No Pixar / AOUSD source is copied**; behavior is reimplemented or stubbed under the `freeusd::` namespace and `include/freeusd/` tree. The FreeUSD codebase is distributed under the **GNU GPL v3.0 or later** (see the repository `LICENSE`); that choice is independent of upstream OpenUSD’s license.

## Namespaces and headers

| OpenUSD convention | FreeUSD convention |
| --- | --- |
| `pxr::` (C++), headers under `pxr/` | `freeusd::`, headers under `include/freeusd/` |
| `USD_API` / `ARCH_API` style export macros | `FREEUSD_API` in [`include/freeusd/export.hpp`](../include/freeusd/export.hpp) |
| `TF_*`, `SDF_*` diagnostics (subset) | [`diagnostic.hpp`](../include/freeusd/diagnostic.hpp), [`source_location.hpp`](../include/freeusd/source_location.hpp) |

## Core stack (implemented subset)

These map directly to the usual **base → Sdf → Pcp → Usd** dependency direction in OpenUSD:

| OpenUSD area (typical `pxr/` path) | FreeUSD path | CMake alias |
| --- | --- | --- |
| `base/tf` | `include/freeusd/tf/` | `freeusd::tf` |
| `base/gf` | `include/freeusd/gf/` (`vec3d`, `matrix4d`, **`bbox3d`**, **`quatd`**, **`range1d`**, …) | `freeusd::gf` |
| `base/vt` | `include/freeusd/vt/` | `freeusd::vt` |
| `usd/ar` | `include/freeusd/ar/` (`defaultResolver`, **`resolvedPath`**, …) | `freeusd::ar` |
| `usd/sdf` | `include/freeusd/sdf/` (`path`, `layer`, **`layerOffset`**, **`assetPath`**, **`tokens`**, …) | `freeusd::sdf` |
| `usd/pcp` | `include/freeusd/pcp/` | `freeusd::pcp` |
| `usd/usd` | `include/freeusd/usd/` (`stage`, `prim`, **`kindTokens`**, **`tokens`**, **`timeCode`**, **`editTarget`**, `crateFile` placeholder) | `freeusd::usd` |
| `usd/usdUtils` | `include/freeusd/usdUtils/` (`pipeline` placeholder) | `freeusd::usdUtils` |
| `usd/usdGeom` | `include/freeusd/usdGeom/` | `freeusd::usdGeom` |
| `usd/usd/crateFile` (binary crate) | `include/freeusd/usd/crateFile.hpp` | (placeholder) |
| `usd` I/O (ASCII in upstream split) | `include/freeusd/io/usda.hpp` | `freeusd::io` |

## Schema libraries (stubs today)

OpenUSD ships many **Usd* schema** libraries (`usdShade`, `usdLux`, …). FreeUSD adds **header-only token stubs** under the same relative names so build scripts and includes can mirror upstream layout. Stubs carry **no** generated schema logic from Pixar; they only expose common `TfToken`-shaped names used in USDA and composition discussions.

| OpenUSD library | FreeUSD include | CMake target |
| --- | --- | --- |
| `usdGeom` (partial) | `usdGeom/` (tokens + `Xformable` stub) | `freeusd::usdGeom` |
| `usdShade` | `usdShade/tokens.hpp` | `freeusd::usdShade` |
| `usdLux` | `usdLux/tokens.hpp` | `freeusd::usdLux` |
| `usdPhysics` | `usdPhysics/tokens.hpp` | `freeusd::usdPhysics` |
| `usdVol` | `usdVol/tokens.hpp` | `freeusd::usdVol` |
| `usdSkel` | `usdSkel/tokens.hpp` | `freeusd::usdSkel` |
| `usdMedia` | `usdMedia/tokens.hpp` | `freeusd::usdMedia` |
| `usdRender` | `usdRender/tokens.hpp` | `freeusd::usdRender` |
| `usdRi` | `usdRi/tokens.hpp` | `freeusd::usdRi` |

Aggregate (optional dependency): `freeusd::usd_schemas` → pulls all of the above for tools that want a single link line similar to “all USD schemas”. Optional umbrella include: [`include/freeusd/usd/schemaTokens.hpp`](../include/freeusd/usd/schemaTokens.hpp).

## Not in scope (upstream layout reference only)

Typical OpenUSD trees that **do not** have FreeUSD counterparts yet (documented so expectations stay clear):

- **Imaging / Hydra / Hgi / Hd*** — render delegate stack.
- **UsdImaging**, **UsdHydra** — scene adapter / hydra schema bridges.
- **Ndr / Sdr** — shader discovery and parsing.
- **UsdMtlx**, **UsdUI**, **UsdUI**-adjacent packages, **exec** / **trace** full implementations beyond stubs.

When work starts in an area, prefer **new files under the matching `include/freeusd/<UsdPackage>/` name** rather than inventing parallel naming.

## CMake parity

- OpenUSD often exposes `usd_ms`, `usdGeom`, … as linkable targets. FreeUSD exposes `freeusd_<name>` with the same **suffix** (`usdGeom` → `freeusd_usdGeom`) to keep `find_package`-style strings predictable after a `s/usd_/freeusd_/` prefix.
- The umbrella **`freeusd::runtime`** target remains the **minimal** link set for the C ABI and Python extension. Optional schema aggregates are **opt-in** via `freeusd::usd_schemas`.

## Python layout

- Schema stubs follow OpenUSD-style **`freeusd.<UsdPackage>.tokens`** (thin wrappers over **`freeusd._native.<UsdPackage>.tokens`**) for `usdGeom`, `usdShade`, `usdLux`, `usdPhysics`, `usdVol`, `usdSkel`, `usdMedia`, `usdRender`, and `usdRi`.
- **`freeusd.sdf.AssetPath`** matches the **`SdfAssetPath`** role for typed authored asset strings (layer APIs may still use plain strings where they always have).
- **`freeusd.ar.ResolvedPath`** matches the **`ArResolvedPath`** role for post-resolution paths; **`DefaultResolver.resolve_path`** wraps **`resolve`**.
- **`freeusd.sdf.builtin_tokens`** exposes common **layer / prim metadata** field names as **`tf.Token`** helpers (SdfTokens-shaped layout; not generated from upstream).
- **`freeusd.usd.builtin_tokens`** exposes common **composition / variant / relocate** field names as **`tf.Token`** helpers (UsdTokens-shaped layout; clean-room).
- Model **`kind`** metadata: **`freeusd.usd.kind_tokens`** → **`freeusd._native.usd.kind_tokens`** (aligned with **`UsdKindTokens`**-style naming, not prim schema type tokens).
- **`freeusd.usd.TimeCode`** mirrors **`UsdTimeCode`**-style default / earliest / numeric authoring; **`freeusd.usdUtils`** exposes placeholder types (e.g. **`FlattenOptions`**) until real utils land.
- **`freeusd.gf`** wraps **`_native.gf`** (`Vec3d`, `Matrix4d`, **`BBox3d`**, **`Quatd`**, **`Range1d`**) for the same **Gf**-shaped role as `pxr.Gf` in OpenUSD stacks.
- **`freeusd.vt.Value`** exposes scalar **`holds_*` / `as_*`** helpers; **`as_double`** still uses **`GetDouble`**, which promotes stored integers and floats to **`double`** where the C++ API allows.
- **`freeusd.plug`**, **`freeusd.work`**, **`freeusd.trace`** mirror **`pxr` plug / work / trace** entry points at the Python package level (see **`_native`** submodules).

## References

- [openusd-map.md](openusd-map.md) — feature-level parity notes.
- [bindings/README.md](../bindings/README.md) — FFI layout (Go / Rust) over the C ABI.
