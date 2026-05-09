# OpenUSD module map (FreeUSD)

FreeUSD mirrors the **library boundaries** and common **public concepts** of OpenUSD (pxr) for interoperability and learning. Implementations are **clean-room** and intentionally smaller until features land.

| OpenUSD (pxr) area | FreeUSD | Notes |
| --- | --- | --- |
| `tf` (tokens, hashing) | `freeusd::tf` | `Token` interning; `HashString` for maps |
| `gf` (linear algebra) | `freeusd::gf` | `Vec3d`, `Matrix4d` containers |
| `vt` (typed values) | `freeusd::vt` | `Value` variant for layer payloads |
| `ar` (asset resolution) | `freeusd::ar` | `Resolver` + `DefaultResolver` (filesystem) |
| `sdf` (scene description) | `freeusd::sdf` | `Path`, `Layer`, `FieldOpinion`, prim **`customData`**, **attribute connections** (`*.connect`), prim **references**, **`inherits` / `specializes` / `payload`** arc lists (authored order; USDA `prepend`/`append`/`=` list ops in prim metadata), prim **relationships** (`Set`/`Prepend`/`Append`/`DeleteRelationshipTargets`) |
| `pcp` (composition) | `freeusd::pcp` | `LayerStack`, **`ComposeSublayers`** / **`ComposeSublayersDepthFirst`**, **`ResolveFieldAtTimeStrongestWins`** (layer-stack falloff) |
| `usd` (stage graph) | `freeusd::usd` | **`Stage::AttachLayerStack`**: ordered layers (strong→weak), attributes with **strongest opinion**; **attribute `.connect`**: follows **first authored connection** from strong→weak per hop, then reads the target property (cycle-limited); **relationship targets concatenate** when each layer contributes a list; **prim kind / active** / **`customData`** as above |
| `usdGeom` | `freeusd::usdGeom` | Schema tokens + `Xformable` stub |
| `plug` | `freeusd::plug` | `Registry` no-op loader |
| `trace` | `freeusd::trace` | No-op `Collector` + `FREEUSD_TRACE_FUNCTION` |
| `work` | `freeusd::work` | Serial `Dispatcher` stub |
| **C ABI** | `libfreeusd_c` + `include/freeusd/c/freeusd.h` | Stable C entry points: layer + **layer stack** + stage attach, USDA I/O, field reads (**double/bool/int64**/string), **field-opinion** query, child paths + **concatenated relationship targets**, composed **active/kind/customData** |
| Crate (binary) | `freeusd::usd::crate` | Header placeholder |
| USDA (ASCII) | `freeusd::io::usda` | Minimal load/save (`def`, typed attrs, nested blocks, prim **`customData = { … }`**, typed **`name.connect = </Prim.attr>`** attribute connections, relationship list ops **`prepend` / `append` / `delete rel`**) |

This is **not** API-compatible with Pixar’s OpenUSD at the ABI or exhaustive method level; it tracks **roles** and a **usable subset** so the codebase can grow toward spec compliance without importing upstream sources.
