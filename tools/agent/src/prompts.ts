/**
 * Shared agent instructions for local and cloud runs.
 * Keep in sync with CONTRIBUTING.md and docs/compatibility-claims.md.
 */

export const REPO_AGENT_RULES = `You are working on FreeUSD, a clean-room GPL-3.0-or-later OpenUSD-shaped scene core.

## Non-negotiable rules

1. Do NOT copy Pixar / AOUSD source code into this repository.
2. Prefer repo-owned fixtures in tests/fixtures/ over broad compatibility claims.
3. Update docs/openusd-parity-matrix.md when a supported status meaningfully changes.
4. Keep engine-facing behavior inside docs/engine-supported-subset.md unless that contract is updated first.
5. Add or reuse a named fixture when a new claim needs a stable validation anchor.

## Allowed claim wording

- "implemented in the current supported subset"
- "partial", "token-only", "intentionally deferred"
- "validated by fixture <name>"

## Disallowed claim wording

- "full OpenUSD parity" or "drop-in replacement"
- broad .usdc support beyond validated table sections and the narrow embedded-USDA fallback

## Acceptance criteria (parity work)

- New behavior: add or reuse a named fixture under tests/fixtures/.
- At least one native test; add binding-facing tests when FFI changes.
- C ABI additions: document ownership in include/freeusd/c/freeusd.h.
- Run cmake + ctest before finishing; all tests must pass.

## Build and verify

From the repository root:

  cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DFREEUSD_BUILD_TESTS=ON
  cmake --build build --parallel
  ctest --test-dir build --output-on-failure

For Python coverage:

  cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \\
    -DFREEUSD_BUILD_PYTHON=ON -DFREEUSD_BUILD_TESTS=ON
  cmake --build build
  ctest --test-dir build --output-on-failure
`;

export function wrapTaskPrompt(taskBody: string): string {
  return `${REPO_AGENT_RULES}

---

## Task

${taskBody}`;
}
