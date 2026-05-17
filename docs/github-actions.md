# GitHub Actions and CI

FreeUSD uses [`.github/workflows/ci.yml`](../.github/workflows/ci.yml) for merge-quality checks. Optional cloud agents use [`.github/workflows/cursor-agent.yml`](../.github/workflows/cursor-agent.yml) (`workflow_dispatch` only).

## If every job fails in 1–3 seconds with no logs

GitHub often shows:

> The job was not started because your account is locked due to a billing issue.

That is an **organization or enterprise billing lock**, not a compile or test failure. No workflow change in this repository can start hosted runners until billing is resolved.

### Fix (organization owner required)

1. Open **[gopexllc billing settings](https://github.com/organizations/gopexllc/settings/billing)**.
2. If an **Enterprise trial** expired, either:
   - **Downgrade** the organization to **GitHub Free** (public repos keep free standard Actions minutes), or
   - **Update payment** and keep the paid plan you intend to use.
3. Remove stale payment methods or failed authorization holds if billing shows $0 but the account stays locked. See [Unlocking a locked account](https://docs.github.com/en/billing/how-tos/troubleshooting/locked-account).
4. After unlock, re-run CI: **Actions → CI → Run workflow**, or push an empty commit.

CodeQL checks that run at the org level fail for the same billing reason until the account unlocks.

### Verify the codebase without GitHub

From the repository root:

```bash
chmod +x scripts/run_ci_locally.sh
./scripts/run_ci_locally.sh
```

This runs tree-hygiene, the Release C++ build + `ctest`, and (when tools are installed) Python, Rust, and Go slices matching CI.

## What CI runs when billing is healthy

| Job | Runner | Purpose |
|-----|--------|---------|
| `tree-hygiene` | `ubuntu-latest` | No tracked `build-*` paths; engine contract docs |
| `linux` | `ubuntu-latest` | C++ tests + install integration |
| `linux-python` | `ubuntu-latest` | Python extension + `ctest` + pytest |
| `linux-bindings` | `ubuntu-latest` | `cargo test`, `go test` |
| `windows` | `windows-latest` | MSVC Release build + `ctest` |
| `macos` | `macos-latest` | Release build + `ctest` |

## Self-hosted runners (optional)

If hosted runners stay blocked but your org allows self-hosted runners, register a runner on your machine and use a workflow that sets `runs-on: self-hosted`. Hosted-runner billing locks may still block runner registration; confirm in org **Settings → Actions → Runners**.

## Cursor agent workflow

[`cursor-agent.yml`](../.github/workflows/cursor-agent.yml) requires secret `CURSOR_API_KEY` and manual dispatch. It does not gate merges.
