# Agent guide (FreeUSD)

Short onboarding for Cursor agents and the `tools/agent` SDK harness. Full rules live in [CONTRIBUTING.md](CONTRIBUTING.md).

## Clean room

- Do not copy Pixar / AOUSD source into this repository.
- Implement from published formats and independent tests only.

## Parity workflow

1. Read [docs/openusd-parity-matrix.md](docs/openusd-parity-matrix.md) for current status (`implemented`, `partial`, `token-only`, `planned`, `intentionally deferred`).
2. Add or reuse a named fixture under `tests/fixtures/` for new behavior.
3. Add native tests; add Python/Go/Rust tests when FFI changes.
4. Update the parity matrix when status meaningfully changes.
5. Run `ctest` before claiming completion.

## Build (typical)

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DFREEUSD_BUILD_TESTS=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

## Cursor SDK CLI

From [tools/agent/](tools/agent/README.md):

```bash
cd tools/agent && npm install
export CURSOR_API_KEY=...
npm run agent:parity -- specs          # local: next USDC SPECS slice
npm run agent:fix-ctest -- --file /tmp/ctest.log
npm run agent:resume -- <agentId> "follow-up prompt"
```

Agents propose changes; **CI and ctest remain the source of truth** for merge quality.

## Claims

Follow [docs/compatibility-claims.md](docs/compatibility-claims.md). Never claim full OpenUSD parity or arbitrary `.usdc` decode beyond tested fixtures.
