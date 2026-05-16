# FreeUSD Security Best Practices Report

**Date:** 2026-05-16  
**Scope:** `/home/tim/freeusd` — C++17 core, C ABI, Python/Go/Rust FFI, untrusted USDA/USDC on disk  
**Method:** Codex `security-best-practices` skill (report mode) + targeted code audit and fixes

## Executive summary

FreeUSD is a **local file parser and composition library**, not a network service. The highest risks are **path traversal** when resolving sublayers/references from untrusted USD packages, **memory exhaustion** from unbounded file reads, **USDA structure injection** via variant payloads, and **integer overflow / out-of-bounds reads** in USDC TOC handling.

This pass **fixed four high-severity issues** (path containment, variant payload brace validation, USDA/USDC file size caps). USDC section parsing already had solid bounds checks; the C ABI TOC name copy is correctly bounded. **Residual risk remains**: TOCTOU on filesystem opens, unbounded `LoadFromString` when callers pass huge in-memory strings, no mutual exclusion on shared `Stage`/`Layer` objects, and incomplete USDC decoding (by design). No review can guarantee absence of exploits.

### Reference documents loaded

The skill’s `references/` directory has **no C++ or native-code guides**. The following were read for general backend hygiene (limited direct applicability):

| File | Relevance |
|------|-----------|
| `golang-general-backend-security.md` | FFI/bindings hygiene, denial-of-service limits |
| `python-fastapi-web-server-security.md` | N/A (no web server) |
| `python-django-web-server-security.md` | N/A |
| `python-flask-web-server-security.md` | N/A |
| `javascript-express-web-server-security.md` | N/A |
| `javascript-typescript-nextjs-web-server-security.md` | N/A |
| `javascript-general-web-frontend-security.md` | N/A |
| `javascript-jquery-web-frontend-security.md` | N/A |
| `javascript-typescript-react-web-frontend-security.md` | N/A |
| `javascript-typescript-vue-web-frontend-security.md` | N/A |

General secure-coding guidance from the skill (avoid public sequential IDs, do not flag missing TLS for local tools) was applied where relevant.

---

## Findings summary

| Severity | Total | Fixed this pass | Deferred / accepted |
|----------|------:|----------------:|--------------------:|
| Critical | 0 | 0 | 0 |
| High | 4 | 4 | 0 |
| Medium | 6 | 1 | 5 |
| Low | 5 | 0 | 5 |

---

## Critical

*(none identified)*

---

## High

### SEC-001 — Path traversal via `DefaultResolver` (fixed)

**Impact:** A malicious sublayer path such as `../../../../etc/passwd` could resolve outside the asset root when an anchor was set.

**Location (before fix):** `src/ar/defaultResolver.cpp` used `weakly_canonical` without verifying the result stayed under the anchor directory.

**Fix:** `ResolvePathUnderAnchor` in `src/ar/pathSecurity.cpp` canonicalizes and rejects paths not contained under the anchor. `DefaultResolver` now stores a canonical anchor and uses this helper.

```11:24:src/ar/defaultResolver.cpp
std::string DefaultResolver::Resolve(std::string_view asset_path) {
  if (asset_path.empty()) {
    return {};
  }
  if (!anchor_.empty()) {
    return ResolvePathUnderAnchor(anchor_, std::string{asset_path});
  }
  return CanonicalizeFilesystemPath(std::string{asset_path});
}
```

### SEC-002 — Path traversal in stage asset / sublayer resolution (fixed)

**Impact:** `subLayers`, references, and payloads could open arbitrary readable files on the host when opening an untrusted root layer.

**Locations:** `resolve_asset_relative_to_layer_identifier`, `Stage::OpenFromRootFile` sublayer `resolve` lambda in `src/usd/stage.cpp`.

**Fix:** Asset resolution uses `ResolvePathUnderAnchor`; sublayer opens re-check `PathIsWithinRootDirectory` after canonicalization.

### SEC-003 — USDA injection via variant payload brace escape (fixed)

**Impact:** Variant set payload text is wrapped in a synthetic `def Scope "__VariantRoot" { ... }`. A payload starting with `}` could close the scope early and inject sibling prims (composition / data confusion).

**Location:**

```286:318:src/usd/stage.cpp
bool variant_payload_stays_within_wrapper(std::string_view body) {
  int depth = 1;
  // ...
}
std::shared_ptr<freeusd::usd::Stage> build_variant_stage_from_payload(const std::string& payload_body) {
  if (!variant_payload_stays_within_wrapper(payload_body)) {
    return {};
  }
  // ...
}
```

### SEC-004 — Unbounded USDA file read (fixed)

**Impact:** `LoadFromFile` read the entire file into a `std::string` with no size cap (denial-of-service via multi-gigabyte `.usda`).

**Location:** `src/io/usda.cpp` — now rejects files larger than `kMaxUsdaLayerFileBytes` (64 MiB) before buffering.

---

## Medium

### SEC-005 — Unbounded USDC crate file size (fixed)

**Impact:** `tellg()`-sized reads could attempt huge allocations for pathological `.usdc` files.

**Fix:** `crate_file_size_ok()` enforces `kMaxUsdcCrateFileBytes` (256 MiB) in bootstrap/TOC paths (`src/usd/crateFile.cpp`).

### SEC-006 — TOCTOU between existence check and open (deferred)

**Impact:** A race could swap a file after `exists()` / canonicalization and before `LoadFromFile` (classic filesystem TOCTOU).

**Mitigation today:** Fail closed on resolution errors; containment reduces cross-directory swaps. **Recommendation:** open with `O_NOFOLLOW` where platform APIs allow, or open once and parse from fd (larger change).

### SEC-007 — Unbounded `LoadFromString` (deferred)

**Impact:** Callers (C ABI, tests, variant wrapper) can pass arbitrarily large in-memory USDA without the file-size guard.

**Recommendation:** Add `max_bytes` to `LoadFromString` or document caller responsibility.

### SEC-008 — Symlinks and `weakly_canonical` (deferred / partially mitigated)

**Impact:** Canonicalization follows symlinks. Containment checks compare canonical paths, so a symlink **outside** the anchor should still be rejected after resolution.

**Residual:** Symlinks **within** the anchor that point to sensitive paths inside the same tree remain reachable (expected for asset packages). Document for deployers.

### SEC-009 — Incomplete USDC decode (accepted)

**Impact:** Only bootstrap/TOC/section-byte helpers and embedded `USDA` section stage-open are implemented; malformed binary sections mostly affect callers using low-level APIs with their own `max_bytes`.

**Note:** Section reads already validate TOC ranges:

```56:88:src/usd/crateFile.cpp
bool toc_section_range_valid(std::int64_t file_bytes, std::int64_t start, std::int64_t size, std::string* err_out) {
  // overflow-safe [start, start+size) ⊆ [0, file_bytes)
}
```

### SEC-010 — Composition re-entrancy guards are thread-local only (deferred)

**Impact:** `static thread_local std::unordered_set` cycle guards in `src/usd/stage.cpp` do not protect cross-thread concurrent use of one `Stage`.

**Documented:** `include/freeusd/c/freeusd.h` states layers/stages are not thread-safe.

### SEC-011 — `LoadFromString` CPU exhaustion (deferred)

**Impact:** Large but under cap files can still burn CPU in the line-oriented parser (algorithmic complexity not fully bounded).

**Recommendation:** Fuzzing and optional parse timeouts for untrusted inputs in hosting processes.

---

## Low

### SEC-012 — C ABI string outputs (status: adequate)

`dup_cstr` uses `malloc(size+1)` with `memcpy` of full string including NUL. TOC section names use `std::min(s.name.size(), sizeof(name)-1)`. No `strncpy` truncation bugs found.

```222:228:src/c/api.cpp
      const std::size_t n = std::min(s.name.size(), sizeof(arr[i].name) - 1u);
      if (n > 0) {
        std::memcpy(arr[i].name, s.name.data(), n);
      }
```

### SEC-013 — FFI lifetime (deferred documentation)

Go/Rust bindings must free `malloc`'d strings via documented APIs; Rust tests use `from_utf8_lossy` on TOC names (NUL-padded fields). **Recommendation:** document UTF-8 + ownership in `bindings/README.md`.

### SEC-014 — `tellg()` / `streamoff` width (deferred)

Very large files on exotic platforms could mis-report size; caps reduce practical impact.

### SEC-015 — Token pool mutex vs stage data races (deferred)

`tf::Token` interning is synchronized; `Stage`/`Layer` mutation is not. Callers must externalize locking.

### SEC-016 — Sdf `Path` rejects `..` in path strings (positive)

```134:136:src/sdf/path.cpp
  if (norm.find("..") != std::string::npos) {
    return false;
  }
```

This does **not** replace filesystem path containment for `@file@` assets.

---

## Positive controls (existing)

| Area | Control |
|------|---------|
| USDC TOC | Negative sizes rejected; `start+size` overflow-safe; `max_sections` / `max_section_bytes` enforced |
| USDC tables | Entry count and string lengths bounded against payload size |
| Embedded USDC stage open | `ReadUsdCrateUsdaSectionFromPath` capped at 8 MiB (`stage.cpp`) |
| C errors | `thread_local` last error; exceptions caught at C boundary |
| Public path API | `..` rejected in `sdf::Path` |

---

## Appendix — Parity task `parity_custom_data_inherit`

**Status: completed.** Covered by:

- `tests/parity_fixtures_test.cpp` (C++ fixture open + composed `customData`)
- `tests/python/test_stage_compose.py::test_parity_custom_data_and_specifier_through_inherits`
- Fixture: `tests/fixtures/parity_custom_data_inherit.usda`

No additional parity work was required for this security pass.

---

## Fixes applied (this commit)

1. `include/freeusd/ar/pathSecurity.hpp` + `src/ar/pathSecurity.cpp` — anchor containment helpers and size limit constants  
2. `src/ar/defaultResolver.cpp` — anchored resolve hardening  
3. `src/usd/stage.cpp` — asset/sublayer containment; variant payload brace guard  
4. `src/io/usda.cpp` — `LoadFromFile` size cap  
5. `src/usd/crateFile.cpp` — USDC file size cap  
6. `src/CMakeLists.txt` — link `pathSecurity.cpp`; `freeusd_io` → `freeusd_ar`

## Verification

```text
ctest --test-dir build
# 33/33 tests passed
```

---

## Residual risk statement

Hardening reduces **likely** attack surfaces for untrusted USD on disk but does not eliminate memory-safety bugs in native code, logic bugs in composition, or host misconfiguration (world-readable secrets next to assets). Treat untrusted USD like untrusted code/data: sandbox processes, limit OS permissions, and keep inputs inside a dedicated asset root.
