# OpenUSD Parity Matrix

This file is the repo-owned parity baseline for FreeUSD. It records what "parity" means today, which fixtures anchor that claim, and what must be true before later roadmap slices are considered complete.

Status vocabulary:

- `implemented`: exercised in tests and intended as part of the current supported subset.
- `partial`: real behavior exists, but important OpenUSD-shaped gaps remain.
- `token-only`: package/module naming or tokens exist without meaningful runtime behavior.
- `planned`: explicitly targeted next, but not yet represented as runtime behavior.
- `intentionally deferred`: visible gap that should stay out of scope until stronger core parity exists.

## Fixture Corpus

- `tests/fixtures/usd_cross_language.usda`
  Cross-language typed field-read contract and basic layer metadata.
- `tests/fixtures/parity_stack_root.usda`
  Root-file opening, sublayer expansion, strongest/weakest field opinions, and authored `subLayerOffsets`.
- `tests/fixtures/parity_stack_mid.usda`
  Mid-strength layer opinions for file-open and stack fixtures.
- `tests/fixtures/parity_stack_weak.usda`
  Weakest layer opinions for stack and time-sample union checks.
- `tests/fixtures/parity_namespace.usda`
  Namespace-affecting metadata: `relocates`, `prefixSubstitutions`, references, and payloads.
- `tests/fixtures/parity_variants.usda`
  `variantSets` declarations plus `variantSelection` metadata.
- `tests/fixtures/parity_imageable.usda`
  Schema-facing `purpose` / `visibility` behavior for `usdGeom::Imageable`.
- Synthetic `.usdc` buffers in `tests/usd_smoke_test.cpp`, `tests/c_abi_smoke.c`, `bindings/go/freeusd_test.go`, and `bindings/rust/freeusd-sys/src/lib.rs`
  Low-level crate bootstrap, TOC, and section-payload parity anchors.

## Current Matrix

### Formats And Data Model

- `implemented`: USDA load/save, typed scalar/vector/quaternion/matrix values, layer metadata, references/payload/inherits/specializes storage, relationship targets, and time-sample evaluation.
- `partial`: USDC bootstrap parsing, TOC parsing, and raw section-payload reads are available in C++, the C ABI, Python, Go, and Rust.
- `planned`: spec-level `.usdc` payload decode beyond raw section access, plus richer authored default-vs-sample preservation during flattening.

### Composition Semantics

- `implemented`: strongest-wins field reads, concatenated relationship lists, composed field/relationship/prim-path unions, relocated prim-path query behavior, and prefix-substituted reference/payload asset paths.
- `partial`: references/payloads/inherits/specializes remain listed metadata rather than full graph expansion; `variantSelection` and `variantSets` are queryable but not yet graph-applying; `subLayerOffsets` are stored and fixture-owned but not time-remapped during composition.
- `planned`: resolver-aware arc expansion, executable variants, and `subLayerOffsets` remapping.

### Schema And Runtime Helpers

- `implemented`: `usdGeom::Xformable`, `usdGeom::Imageable`, and `usdUtils::FlattenStageAtTime`.
- `partial`: flattening currently captures a composed single-time view, not full authored default/sample fidelity.
- `token-only`: most non-`usdGeom` schema packages remain generated token surfaces only.

### ABI And Bindings

- `implemented`: the C ABI remains the stable FFI contract for stage queries, typed field reads, crate bootstrap/TOC helpers, and raw crate section payload access.
- `partial`: Python is still the broadest wrapper; Go and Rust follow the validated subset and now include the new crate section-payload surface.
- `planned`: milestone-by-milestone expansion for more composition graph behavior once the core semantics land in C++ first.

### Runtime Hardening And Deferred Stacks

- `implemented`: lightweight `plug`, `trace`, and `work` utilities exist for validation and smoke coverage.
- `intentionally deferred`: Hydra/imaging, Ndr/Sdr, and broader runtime ecosystems.

## Acceptance Criteria

### Milestone 1: Parity Baseline

- New parity work adds or reuses a named fixture from `tests/fixtures/`.
- The matrix above is updated when a status meaningfully changes.
- At least one native test and one binding-facing test cover the new behavior when an FFI surface is involved.

### Milestone 2: Formats And Data Model

- USDC work lands first in C++ and is validated with synthetic crate fixtures or checked-in files.
- The C ABI mirrors only the validated crate slice.
- Python, Go, and Rust expose the same validated low-level crate behavior before broader marketing claims change.

### Milestone 3: Composition Semantics

- New graph behavior must affect composed queries, not only stored metadata accessors.
- Relocation/prefix/path semantics must be exercised through `Stage` or `Prim`, not just `Layer`.
- Cross-language tests should verify any behavior that changes public query results.

### Milestone 4: Schema And Runtime APIs

- New schema helpers must read authored scenes and return behavior, not only tokens.
- `usdUtils` additions must be executable utilities with tests, not placeholder structs.

### Milestone 5: ABI And Binding Expansion

- C ABI additions document ownership and failure semantics in `include/freeusd/c/freeusd.h`.
- Python can go first, but Go/Rust should follow once the C ABI slice is stable and tested.

### Milestone 6: Deferred Stacks

- Imaging/Hydra-class work stays out of scope until the matrix rows above move from `partial` to stronger core parity.
