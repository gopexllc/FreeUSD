# Contributing

FreeUSD accepts clean-room contributions only.

Before sending engine-facing or parity work, read:

- `docs/clean-room-policy.md`
- `docs/fixture-policy.md`
- `docs/compatibility-claims.md`
- `docs/engine-supported-subset.md`
- `docs/openusd-parity-matrix.md`

## Ground Rules

1. Do not copy Pixar / AOUSD source code into this repository.
2. Prefer repo-owned fixtures and tests over broad compatibility claims.
3. Update `docs/openusd-parity-matrix.md` when a supported status changes.
4. Keep engine-facing behavior inside the documented subset unless the contract is updated first.
5. Add or reuse a named fixture when a new claim needs a stable validation anchor.
