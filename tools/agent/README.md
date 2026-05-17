# FreeUSD Cursor SDK harness

TypeScript CLI around [`@cursor/sdk`](https://cursor.com/docs/api/sdk/typescript) for parity-driven development. **Agents do not replace CI**; merge quality still depends on [`.github/workflows/ci.yml`](../../.github/workflows/ci.yml) and `ctest`.

See also [AGENTS.md](../../AGENTS.md) for repo-wide agent rules.

## Setup

Requires **Node.js 20+**.

```bash
cd tools/agent
npm install
cp .env.example .env   # optional; export CURSOR_API_KEY instead
export CURSOR_API_KEY=cursor_...
```

Get a key from [Cursor Cloud Agents dashboard](https://cursor.com/dashboard/cloud-agents) or your team service account.

## Local commands

| npm script | Purpose |
|------------|---------|
| `npm run agent:parity -- specs` | Local agent: next USDC **SPECS** slice (default next milestone) |
| `npm run agent:parity -- fields` | Local agent: harden **FIELDS** / production layout |
| `npm run agent:fix-ctest -- --file log.txt` | Fix failing tests from a ctest log |
| `npm run agent:resume -- <agentId> "prompt"` | Resume a previous local agent |
| `npm run build` | Compile `src/` to `dist/` |

Append `--dry-run` to print the composed prompt without calling the API.

### Examples

```bash
# Next parity slice (SPECS)
npm run agent:parity -- specs

# Fix tests after a failed ctest run
ctest --test-dir build --output-on-failure 2>&1 | tee /tmp/ctest.log
npm run agent:fix-ctest -- --file /tmp/ctest.log

# Resume long work
npm run agent:resume -- <agentId> "Add FIELDSETS after SPECS lands"
```

### Exit codes

| Code | Meaning |
|------|---------|
| 0 | Run finished successfully |
| 1 | Startup failure (`CursorAgentError`: auth, config, network) |
| 2 | Run started but ended with `result.status === "error"` |

Local runs log `agentId=` and `runId=` to stderr immediately after `send()`.

## Cloud (manual GitHub Actions)

Workflow: [`.github/workflows/cursor-agent.yml`](../../.github/workflows/cursor-agent.yml)

1. Add repository secret **`CURSOR_API_KEY`**.
2. **Actions → Cursor agent → Run workflow**
3. Choose `task` (`parity-usdc-specs` or `parity-usdc-fields`) and `ref` (branch).

The job runs `npm run agent:cloud-parity` with `Agent.prompt` and `skipReviewerRequest: true`. It does not block merges on other workflows.

Optional env for local cloud-style dry run:

```bash
FREEUSD_AGENT_REPO_URL=https://github.com/gopexllc/FreeUSD.git \
  npm run agent:cloud-parity -- specs --ref main --dry-run
```

## Optional environment

| Variable | Default |
|----------|---------|
| `CURSOR_API_KEY` | (required) |
| `FREEUSD_AGENT_MODEL` | `composer-2` |
| `FREEUSD_AGENT_REPO_URL` | `https://github.com/gopexllc/FreeUSD.git` |
| `FREEUSD_CTEST_LOG` | Used by `fix-ctest` when stdin is empty |
