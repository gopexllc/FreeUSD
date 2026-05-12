# Clean-Room Policy

FreeUSD tracks OpenUSD's public file formats, concepts, and library boundaries without copying Pixar or AOUSD source code. This policy applies to all engine-facing work and all parity work.

## Prohibited Inputs

- Pixar / AOUSD source files or patches
- translated or paraphrased upstream implementations
- code generated from upstream implementation sources
- review comments that paste upstream code into this repository

## Allowed Inputs

- published specifications and file-format documentation
- observed behavior from independently created scenes
- repo-owned tests and fixtures
- generated token/schema data sourced from published `generatedSchema.usda` inputs already documented in this repository
- clean-room notes that describe expected behavior without copying implementation text

## Contribution Rules

1. Start from a repo-owned failing test, fixture, or documented gap.
2. Record the behavior target in `docs/openusd-parity-matrix.md` when the status changes.
3. Prefer fixture-backed validation over prose-only claims.
4. Keep all new engine-facing behavior inside documented public surfaces.
5. Do not widen compatibility claims beyond what tests and fixtures prove.

## Review Rules

1. Reject changes that introduce copied upstream code or suspiciously direct transliterations.
2. Reject engine-facing claims that are not anchored in the parity matrix and a named fixture or test.
3. Reject runtime feature growth when it expands beyond `docs/engine-supported-subset.md` without updating that contract first.
4. Keep `USDC` claims conservative until binary scene decode is test-backed beyond the current narrow fallback.

## Provenance Notes

- Binary fixtures in `tests/fixtures/` must be generated inside this repository from repo-owned scripts or handwritten clean-room payloads.
- New fixtures should carry enough context in commit messages and docs for reviewers to understand where they came from.
- If a contributor is unsure whether a source is safe to consult, stop and use only the already-approved repo docs, fixtures, and public specifications.
