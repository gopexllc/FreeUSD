import { wrapTaskPrompt } from "../prompts.js";

const USDC_SPECS_TASK = `Implement the next USDC crate slice for FreeUSD: **SPECS** table decode (minimal spec records).

Context:
- FIELDS table decode is already on main (ReadUsdCrateFieldsTableFromPath, C ABI, Python/Go/Rust).
- Validated today: bootstrap, TOC, TOKENS/STRINGS/PATHS/FIELDS, embedded-USDA stage-open fallback.
- Matrix row for formats remains partial until SPECS (and later FIELDSETS) land with tests.

Requirements:
1. Extend C++ crate reader under src/usd/ (and headers under include/freeusd/usd/) for a minimal SPECS section:
   path index, field-set index, spec type (match OpenUSD crate layout for the fixture subset; stay clean-room).
2. Add or update tests/fixtures/ (e.g. extend parity_tables.usdc or add parity_specs.usdc) with synthetic crate bytes.
3. Add native test(s) and extend parity_fixtures_test / c_abi_smoke as appropriate.
4. Mirror validated slice in C ABI (include/freeusd/c/freeusd.h) and Python/Go/Rust bindings when the C ABI is stable.
5. Update docs/openusd-parity-matrix.md: move USDC from partial toward stronger partial only if tests pass.
6. Run cmake + ctest; fix failures before finishing.

Do not claim full .usdc interchange or arbitrary production crate support beyond what tests prove.`;

const USDC_FIELDS_TASK = `Extend or harden USDC **FIELDS** crate support in FreeUSD.

Context:
- Structured FIELDS decode (token_index + value_type_token_index per row) exists on main.
- Next within FIELDS: production OpenUSD 0.4+ compressed token-index + value-rep layout (beyond fixture string-table shape).

Requirements:
1. Implement production FIELDS layout in the clean-room crate reader with tests.
2. Keep fixture-backed parity_tables.usdc passing; add fixtures for new layout if needed.
3. C ABI + bindings follow the validated C++ slice.
4. Update docs/openusd-parity-matrix.md only when status meaningfully changes.
5. Run cmake + ctest before finishing.`;

export type UsdcParitySlice = "fields" | "specs";

export function buildUsdcParityPrompt(slice: UsdcParitySlice): string {
  const body = slice === "specs" ? USDC_SPECS_TASK : USDC_FIELDS_TASK;
  return wrapTaskPrompt(body);
}
