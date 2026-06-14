# Agent guide (FreeUSD)

Short onboarding for Cursor agents and the `tools/agent` SDK harness. Full rules live in [CONTRIBUTING.md](CONTRIBUTING.md) and [docs/clean-room-policy.md](docs/clean-room-policy.md).

## North star (GPL stack)

FreeUSD targets a **full OpenUSD-shaped library stack** (formats, composition, schema helpers, FFI) under **GPL-3.0-or-later**, implemented **clean-room** for GPL game-engine integration. It is **not** a drop-in Pixar OpenUSD replacement; claims must follow [docs/compatibility-claims.md](docs/compatibility-claims.md) and [docs/openusd-parity-matrix.md](docs/openusd-parity-matrix.md).

## GPL / clean-room (non-negotiable)

- **Never** copy Pixar / AOUSD source, patches, or transliterations into this repo.
- Implement from **published formats**, repo-owned fixtures, independent tests, and behavior described without upstream implementation text.
- Reject pasted upstream code in review; when unsure about a source, stop and use only approved repo docs and public specifications.
- **Hydra / Hd imaging**, **Ndr / Sdr**, and broad runtime ecosystems stay **intentionally deferred** until core parity rows move from `partial` toward stronger coverage (see matrix).

## Layered roadmap

Work top-down in layers; do not skip validation at a lower layer.

| Layer | OpenUSD-shaped area | FreeUSD focus | Status (see matrix) |
| --- | --- | --- | --- |
| 1 | Formats / data model | USDA I/O, `vt`/`sdf`, USDC crate tables | USDA `implemented`; USDC tables `partial` |
| 2 | Composition (Pcp) | `LayerStack`, arcs, relocates, variants, composed queries | `partial` |
| 3 | Stage / schema runtime | `usdGeom`, `usdSkel`, `usdShade`, `usdLux`, … | Core geom/skel/shade/lux `partial`; most other schema packages `token-only` |
| 4 | Engine contract | `usdUtils`, [engine-supported-subset.md](docs/engine-supported-subset.md) | `partial` |
| 5 | FFI | C ABI, Python, Go, Rust | C ABI + Python broadest; Go/Rust follow validated C slices |
| 6 (deferred) | Imaging | Hydra, UsdImaging, shader registries | **Not present** |

Module boundaries: [docs/openusd-map.md](docs/openusd-map.md). Repo layout: [docs/openusd-repo-alignment.md](docs/openusd-repo-alignment.md).

## Shipped on main vs local WIP

**Recent main commits (parity milestones):**

- `c302408` — USDC `SPECS` table decode; `UsdGeom::Mesh` parity helpers.
- `2942708` — USDC `FIELDSETS`; lux lights; shade textures; mesh primvars.
- `493ca1b` — continual-learning transcript index for agent memory.

**On main today:** USDA load/save and composition subsets are strong; USDC has bootstrap/TOC plus validated `TOKENS` / `STRINGS` / `PATHS` / `FIELDS` / `FIELDSETS` / `SPECS` / `VALUES` on `parity_tables.usdc` (fixture typed kinds Int32/Float/TokenIndex/Bool only). Schema helpers cover mesh, skel (glTF-oriented), preview materials, lux, physics (`PhysicsScene`, `RigidBodyAPI`, `CollisionAPI`, `MassAPI`, `FixedJoint`), and volume families at `partial` depth. Composition includes composed `customData` through references/payloads/inherits/specializes and kind/active through specializes.

**USDC `VALUES` next (matrix `planned`):** arbitrary production `.usdc` typed value kinds and compression beyond `parity_tables.usdc`.

## Agent work sequence (every slice)

1. Read [docs/openusd-parity-matrix.md](docs/openusd-parity-matrix.md) for the target row (`implemented` / `partial` / `planned`).
2. Add or reuse a **named fixture** under `tests/fixtures/`.
3. Implement and test in **C++** first (`src/`, `include/freeusd/`).
4. Run **`ctest`** (and `./scripts/run_ci_locally.sh` when GitHub Actions is blocked).
5. Update the **parity matrix** when status meaningfully changes.
6. Mirror only the validated slice in **`include/freeusd/c/freeusd.h`**, then **Python / Go / Rust** bindings.
7. Do **not** commit or push unless the user explicitly asks; keep unrelated WIP out of commits.

### USDC incremental slices (clean-room)

Work in small, test-backed steps on the shared crate fixture:

1. Regenerate `tests/fixtures/parity_tables.usdc` with `scripts/gen_parity_tables_usdc.py` when table layout changes.
2. Land C++ decode in `src/usd/crateFile.cpp` / `include/freeusd/usd/crateFile.hpp` first.
3. Mirror only the validated slice in `include/freeusd/c/freeusd.h` and bindings.
4. Typical table order: bootstrap/TOC, then `TOKENS` / `STRINGS` / `PATHS` / `FIELDS` / `FIELDSETS` / `SPECS` / `VALUES` (typed fixture kinds), then broader production value kinds (`planned`).
5. Do not claim arbitrary `.usdc` scene decode; production compression and full embedded-USDA bridge remain out of scope until fixture-backed.

### Schema / engine slices

- Prefer one prim family or one composition behavior per slice (e.g. `usdGeom::Mesh`, `usdLux::*Light`, `usdShade` texture connections, `usdSkel` glTF skinning).
- Engine-facing behavior must stay inside [docs/engine-supported-subset.md](docs/engine-supported-subset.md) and [docs/engine-integration.md](docs/engine-integration.md).

## CI and babysit

- **Merge truth:** `ctest` and local CI, not agent prose.
- GitHub org **`gopexllc`** may fail all Actions/CodeQL in 1–3 seconds with a **billing lock**; that is not a compile failure ([docs/github-actions.md](docs/github-actions.md)).
- When Actions is blocked, run `./scripts/run_ci_locally.sh` before reporting work complete.
- For open PRs: triage review comments, resolve conflicts, fix local/CI failures in a loop (see babysit skill if assigned).

## Key paths

| Area | Path |
|------|------|
| Parity baseline | `docs/openusd-parity-matrix.md` |
| Engine contract | `docs/engine-supported-subset.md`, `src/usdUtils/engineScene.cpp` |
| USDC crate | `src/usd/crateFile.cpp`, `tests/usdc_specs_test.cpp`, `tests/usdc_fieldsets_test.cpp`, `tests/usdc_values_test.cpp` |
| Stage / composition | `src/usd/stage.cpp`, `tests/parity_fixtures_test.cpp` |
| C ABI | `include/freeusd/c/freeusd.h`, `src/c/api.cpp` |
| Fixtures | `tests/fixtures/parity_*.usda`, `parity_tables.usdc` |
| Local CI mirror | `scripts/run_ci_locally.sh`, `docs/github-actions.md` |

## Build (typical)

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DFREEUSD_BUILD_TESTS=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

When GitHub-hosted CI is unavailable (org billing lock), run `./scripts/run_ci_locally.sh` instead of treating instant Actions failures as compile errors.

## Cursor SDK CLI

From [tools/agent/](tools/agent/README.md):

```bash
cd tools/agent && npm install
export CURSOR_API_KEY=...
npm run agent:parity -- specs          # local: next USDC SPECS slice
npm run agent:parity -- fields         # local: harden FIELDS / production layout
npm run agent:fix-ctest -- --file /tmp/ctest.log
npm run agent:resume -- <agentId> "follow-up prompt"
```

Agents propose changes; **CI and ctest remain the source of truth** for merge quality.

## Claims

Follow [docs/compatibility-claims.md](docs/compatibility-claims.md). Never claim full OpenUSD parity or arbitrary `.usdc` decode beyond tested fixtures.

## Learned User Preferences

- Build the full OpenUSD-shaped stack under GPL-3.0-or-later with clean-room implementation only (no Pixar/AOUSD source copies).
- Advance behavioral parity incrementally: one fixture-backed slice at a time, not broad refactors.
- Keep C++, C ABI, Python, Go, and Rust feature surfaces aligned after C++ behavior is validated.
- Prioritize glTF-aligned skeleton, bone animation, and morph/blend-shape behavior when working `usdSkel`.
- Run `ctest` and `scripts/run_ci_locally.sh` when GitHub Actions is blocked before reporting work complete.
- Create git commits and pushes only when the user explicitly asks; exclude unrelated WIP from commits.
- Target GPL game-engine integration (importer/editor/runtime subset), not drop-in full OpenUSD replacement.
- Keep USD-to-knowledge-graph and ontology-grounding work inside clean-room, GPL-compatible boundaries; treat papers as research context, not source text or proof of implemented parity.
- Use plain ASCII punctuation in user-facing docs (avoid em dashes).
- Cross-language parity matters: mirror validated C ABI slices in Go/Rust, not only Python.

## Learned Workspace Facts

- GitHub org `gopexllc` may show all CI/CodeQL jobs failing in 1–3 seconds with a billing lock message; that is not a code failure (see [docs/github-actions.md](docs/github-actions.md)).
- Main milestones: `c302408` (USDC `SPECS` + `UsdGeom::Mesh`), `2942708` (USDC `FIELDSETS`, lux/shade/mesh primvars), `493ca1b` (continual-learning index).
- USDC table decode through `SPECS` and fixture `VALUES` (opaque + typed kinds on parity branch).
- Matrix `planned` next for USDC: arbitrary production value kinds, compression, full embedded-USDA bridge.
- Composition, `usdShade`, `usdLux`, and `usdSkel` are `partial` per the parity matrix; Hydra/imaging/Ndr/Sdr are intentionally deferred.
- Shared USDC validation uses `tests/fixtures/parity_tables.usdc` from `scripts/gen_parity_tables_usdc.py`.
- Cross-language contract tests use `tests/fixtures/usd_cross_language.usda` and `tests/parity_fixtures_test.cpp`.
- C/C++ test targets keep runtime assertions enabled in Release builds so `assert`-based tests are meaningful.
- Optional cloud agents use `.github/workflows/cursor-agent.yml` with `CURSOR_API_KEY`; they do not replace merge gates.
- Most non-core schema packages remain token-only until a fixture-backed runtime slice lands.
- USD scene-to-ontology research context highlights prim names, parent paths, siblings, world-frame positions, bounding boxes, mass, and ontology TBox linearization as useful cues; semantic scene graph signals should be tested against geometry-only baselines before claims.

## Cursor Cloud specific instructions

- **Build and test:** See the **Build (typical)** section above. After `cmake --build`, run `ctest --test-dir build --output-on-failure` and `./scripts/run_ci_locally.sh` (covers C++, install smoke, Python pytest, Rust, Go). GitHub Actions for org `gopexllc` may fail instantly with a billing lock; that is not a compile failure.
- **Python tests:** From repo root, `PYTHONPATH=.` and `pytest tests/python` (local CI installs pytest if missing). Parity fixtures live under `tests/fixtures/`; many C tests need `FREEUSD_TEST_FIXTURES_DIR` set in CMake (for example `freeusd_c_stage_variant`).
- **Go bindings:** After C++ changes, run `python3 scripts/patch_go_bindings.py` from repo root if Go link symbols drift; then `go test ./...` under `bindings/go`.
- **Variant metadata through references:** Already covered on `main` via `visit_composition_arc_prim_paths` in `src/usd/stage.cpp` and fixtures `parity_variant_selection_refs.usda` / `parity_variant_sets_refs.usda` (referenced asset `parity_variant_sets_ref.usda`). Composed field reads use separate arc logic in `ReadFieldAtEvaluatedTime`.
