import { Agent, CursorAgentError } from "@cursor/sdk";

import {
  defaultCloudRepoUrl,
  defaultModel,
  normalizeCloudRepoUrl,
  requireApiKey,
} from "./config.js";

export type CloudRunOptions = {
  prompt: string;
  ref?: string;
  repoUrl?: string;
  dryRun?: boolean;
};

export type CloudRunResult = {
  agentId?: string;
  runId?: string;
  status: string;
};

export async function runCloudAgent(
  options: CloudRunOptions,
): Promise<CloudRunResult | null> {
  const ref = options.ref?.trim() || "main";
  const repoUrl = normalizeCloudRepoUrl(
    options.repoUrl?.trim() || defaultCloudRepoUrl(),
  );

  if (options.dryRun) {
    console.log(`--- dry-run cloud (ref=${ref}, repo=${repoUrl}) ---\n`);
    console.log(options.prompt);
    console.log("\n--- end prompt ---");
    return null;
  }

  try {
    const result = await Agent.prompt(options.prompt, {
      apiKey: requireApiKey(),
      model: defaultModel(),
      cloud: {
        repos: [{ url: repoUrl, startingRef: ref }],
        skipReviewerRequest: true,
      },
    });

    if (result.status === "error") {
      console.error(`cloud run failed: status=${result.status}`);
      process.exitCode = 2;
    }

    return {
      status: result.status,
    };
  } catch (err) {
    if (err instanceof CursorAgentError) {
      console.error(
        `cloud startup failed: ${err.message} retryable=${err.isRetryable}`,
      );
      process.exitCode = 1;
      return null;
    }
    throw err;
  }
}
