# Fixture Policy

Fixtures are part of the FreeUSD public contract. They are not throwaway test data.

## What Fixtures Must Prove

- one named behavior slice from `docs/openusd-parity-matrix.md`
- one engine-visible claim from `docs/engine-supported-subset.md`, when the fixture is used in engine-facing tests
- deterministic behavior across C++, the C ABI, and any mirrored binding tests that claim the same support

## Fixture Sources

- Prefer handwritten USDA or repo-generated binary payloads.
- If a fixture is derived from an external format description, the implementation must still be created clean-room in this repository.
- Do not check in fixtures copied from Pixar / AOUSD source trees or test corpora.

## Naming

- Use `parity_*.usda` or `parity_*.usdc` for parity and engine-contract anchors.
- Keep names stable once they are referenced by docs or public claims.

## Binary Fixture Rules

- Keep the encoding narrow and documented.
- Add at least one native test that explains what the binary fixture is validating.
- If the fixture depends on a custom bridge format, document that fact plainly so readers do not confuse it with full spec parity.

## When To Add A Fixture

- add one when a new engine/runtime claim needs a stable anchor
- add one when a bug fix changes composed query behavior
- reuse an existing fixture when it already covers the behavior cleanly

## When Not To Add A Fixture

- do not add low-signal duplicates
- do not add fixtures for behavior that is still intentionally unsupported
