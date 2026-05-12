# Compatibility Claims

FreeUSD is a USD-shaped scene core, not a drop-in replacement for Pixar OpenUSD.

## Required Evidence For Claims

Every compatibility, parity, or engine-readiness claim must point to:

1. a status in `docs/openusd-parity-matrix.md`
2. a named fixture in `tests/fixtures/` or a native test that exercises the exact behavior
3. `docs/engine-supported-subset.md` when the claim is about engine tools, editor, or runtime use

## Allowed Wording

- "implemented in the current supported subset"
- "partial"
- "token-only"
- "intentionally deferred"
- "validated by fixture `<name>`"

## Disallowed Wording

- "full OpenUSD parity"
- "drop-in replacement"
- "runtime safe" without stating the runtime mode
- broad `.usdc` support claims beyond the current validated low-level tables and narrow embedded-USDA fallback

## Engine-Specific Claims

- Tools/editor claims may rely on `freeusd::runtime`, `Stage`, `Prim`, and the validated `usdGeom` / `usdUtils` helpers.
- Runtime claims must state which mode applies:
  - `pre_baked_assets_only`
  - `hybrid_metadata`
  - `experimental_live_stage`

## Review Shortcut

If a claim cannot be traced to a parity-matrix row and a named fixture, it should be rewritten or removed.
