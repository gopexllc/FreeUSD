import { readFileSync } from "node:fs";

import { wrapTaskPrompt } from "../prompts.js";

export function readCtestFailureInput(filePath?: string): string {
  if (filePath) {
    return readFileSync(filePath, "utf8");
  }
  const fromEnv = process.env.FREEUSD_CTEST_LOG?.trim();
  if (fromEnv) {
    return fromEnv;
  }
  if (!process.stdin.isTTY) {
    return readFileSync(0, "utf8");
  }
  throw new Error(
    "Provide ctest output via stdin, FREEUSD_CTEST_LOG env, or --file <path>",
  );
}

export function buildFixCtestPrompt(failureLog: string): string {
  return wrapTaskPrompt(`Fix the failing FreeUSD tests shown below.

1. Reproduce with cmake + ctest from the repository root.
2. Fix the root cause with minimal, focused changes (no unrelated refactors).
3. Add or update a regression test if the failure was not already covered.
4. Ensure all ctest targets pass before finishing.
5. Respect clean-room rules; do not copy upstream OpenUSD sources.

## ctest failure log

\`\`\`
${failureLog.trim()}
\`\`\``);
}
