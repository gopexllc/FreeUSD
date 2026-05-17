import { execSync } from "node:child_process";
import { existsSync } from "node:fs";
import { resolve } from "node:path";

const DEFAULT_REPO_URL = "https://github.com/gopexllc/FreeUSD.git";

export function resolveRepoRoot(): string {
  try {
    return execSync("git rev-parse --show-toplevel", {
      encoding: "utf8",
    }).trim();
  } catch {
    const cwd = process.cwd();
    if (existsSync(resolve(cwd, "CMakeLists.txt"))) {
      return cwd;
    }
    throw new Error(
      "Could not resolve repository root (run from inside the freeusd git checkout)",
    );
  }
}

export function requireApiKey(): string {
  const key = process.env.CURSOR_API_KEY?.trim();
  if (!key) {
    throw new Error(
      "CURSOR_API_KEY is required. Export it or copy tools/agent/.env.example",
    );
  }
  return key;
}

export function defaultModel(): { id: string } {
  return { id: process.env.FREEUSD_AGENT_MODEL?.trim() || "composer-2" };
}

export function defaultCloudRepoUrl(): string {
  const fromEnv = process.env.FREEUSD_AGENT_REPO_URL?.trim();
  if (fromEnv) return fromEnv;
  const gh = process.env.GITHUB_REPOSITORY?.trim();
  if (gh) return `https://github.com/${gh}.git`;
  return DEFAULT_REPO_URL;
}

export function normalizeCloudRepoUrl(url: string): string {
  const trimmed = url.trim();
  if (trimmed.startsWith("https://") || trimmed.startsWith("http://")) {
    return trimmed.endsWith(".git") ? trimmed : `${trimmed}.git`;
  }
  if (/^[\w.-]+\/[\w.-]+$/.test(trimmed)) {
    return `https://github.com/${trimmed}.git`;
  }
  return trimmed;
}
