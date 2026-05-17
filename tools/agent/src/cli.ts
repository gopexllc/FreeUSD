#!/usr/bin/env node
import { buildFixCtestPrompt, readCtestFailureInput } from "./tasks/fixCtest.js";
import { buildUsdcParityPrompt, type UsdcParitySlice } from "./tasks/parityUsdc.js";
import { runCloudAgent } from "./runCloud.js";
import { resumeLocalAgent, runLocalAgent } from "./runLocal.js";

function usage(): void {
  console.log(`freeusd-agent — Cursor SDK harness for FreeUSD

Usage:
  npm run agent -- parity <fields|specs> [--dry-run]
  npm run agent -- fix-ctest [--file <path>] [--dry-run]
  npm run agent -- resume <agentId> "<prompt>" [--dry-run]
  npm run agent -- cloud-parity <fields|specs> [--ref <branch>] [--repo <url>] [--dry-run]

Environment:
  CURSOR_API_KEY          Required (unless --dry-run)
  FREEUSD_AGENT_MODEL     Model id (default: composer-2)
  FREEUSD_AGENT_REPO_URL  Cloud clone URL (default: github.com/gopexllc/FreeUSD.git)
  FREEUSD_CTEST_LOG       ctest output for fix-ctest when not using stdin/--file

Exit codes:
  0  finished
  1  startup failed (CursorAgentError)
  2  run finished with error status
`);
}

function flagValue(args: string[], name: string): string | undefined {
  const i = args.indexOf(name);
  if (i === -1 || i + 1 >= args.length) return undefined;
  return args[i + 1];
}

function hasFlag(args: string[], name: string): boolean {
  return args.includes(name);
}

function parseUsdcSlice(raw: string | undefined): UsdcParitySlice {
  if (raw === "specs" || raw === "usdc-specs") return "specs";
  if (raw === "fields" || raw === "usdc-fields") return "fields";
  throw new Error(`Unknown USDC slice "${raw}". Use: fields | specs`);
}

async function main(): Promise<void> {
  const argv = process.argv.slice(2);
  if (argv.length === 0 || argv.includes("-h") || argv.includes("--help")) {
    usage();
    process.exit(argv.length === 0 ? 1 : 0);
  }

  const dryRun = hasFlag(argv, "--dry-run");
  const cmd = argv[0];

  if (cmd === "parity") {
    const slice = parseUsdcSlice(argv[1]);
    const prompt = buildUsdcParityPrompt(slice);
    await runLocalAgent({ prompt, dryRun });
    return;
  }

  if (cmd === "fix-ctest") {
    const file = flagValue(argv, "--file");
    const prompt = buildFixCtestPrompt(readCtestFailureInput(file));
    await runLocalAgent({ prompt, dryRun });
    return;
  }

  if (cmd === "resume") {
    const agentId = argv[1];
    if (!agentId) {
      throw new Error("resume requires <agentId> and a prompt string");
    }
    const promptStart = argv.indexOf(agentId) + 1;
    const prompt = argv.slice(promptStart).filter((a) => a !== "--dry-run").join(" ");
    if (!prompt) {
      throw new Error('resume requires a prompt, e.g. resume bc-abc "continue SPECS work"');
    }
    await resumeLocalAgent(agentId, prompt, dryRun);
    return;
  }

  if (cmd === "cloud-parity") {
    const slice = parseUsdcSlice(argv[1]);
    const ref = flagValue(argv, "--ref");
    const repo = flagValue(argv, "--repo");
    const prompt = buildUsdcParityPrompt(slice);
    await runCloudAgent({ prompt, ref, repoUrl: repo, dryRun });
    return;
  }

  throw new Error(`Unknown command: ${cmd}. Run with --help.`);
}

main().catch((err: unknown) => {
  const message = err instanceof Error ? err.message : String(err);
  console.error(message);
  process.exit(1);
});
