# OpenUSD module map (FreeUSD)

FreeUSD mirrors the **library boundaries** and common **public concepts** of OpenUSD (pxr) for interoperability and learning. Implementations are **clean-room** and intentionally smaller until features land.

| OpenUSD (pxr) area | FreeUSD | Notes |
| --- | --- | --- |
| `tf` (tokens, hashing) | `freeusd::tf` | `Token` interning; `HashString` for maps |
| `gf` (linear algebra) | `freeusd::gf` | `Vec3d`, `Matrix4d` containers |
| `vt` (typed values) | `freeusd::vt` | `Value` variant for layer payloads |
| `ar` (asset resolution) | `freeusd::ar` | `Resolver` + `DefaultResolver` (filesystem) |
| `sdf` (scene description) | `freeusd::sdf` | `Path`, `FieldOpinion` (default + time samples), in-memory `Layer` |
| `pcp` (composition) | `freeusd::pcp` | `LayerStack` ordering only |
| `usd` (stage graph) | `freeusd::usd` | `Stage`, `Prim` over a single root layer |
| `usdGeom` | `freeusd::usdGeom` | Schema tokens + `Xformable` stub |
| `plug` | `freeusd::plug` | `Registry` no-op loader |
| `trace` | `freeusd::trace` | No-op `Collector` + `FREEUSD_TRACE_FUNCTION` |
| `work` | `freeusd::work` | Serial `Dispatcher` stub |
| Crate (binary) | `freeusd::usd::crate` | Header placeholder |
| USDA (ASCII) | `freeusd::io::usda` | Minimal load/save (`def`, typed attrs, nested blocks) |

This is **not** API-compatible with Pixar’s OpenUSD at the ABI or exhaustive method level; it tracks **roles** and a **usable subset** so the codebase can grow toward spec compliance without importing upstream sources.
