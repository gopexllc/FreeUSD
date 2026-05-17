import { Agent, CursorAgentError } from "@cursor/sdk";
import type { AgentOptions } from "@cursor/sdk";

import { defaultModel, requireApiKey, resolveRepoRoot } from "./config.js";

export type LocalRunOptions = {
  prompt: string;
  dryRun?: boolean;
  onStdout?: (text: string) => void;
};

export type LocalRunResult = {
  agentId: string;
  runId: string;
  status: string;
};

function buildAgentOptions(repoRoot: string): AgentOptions {
  return {
    apiKey: requireApiKey(),
    model: defaultModel(),
    local: {
      cwd: repoRoot,
      settingSources: [],
    },
  };
}

export async function runLocalAgent(
  options: LocalRunOptions,
): Promise<LocalRunResult | null> {
  if (options.dryRun) {
    console.log("--- dry-run prompt ---\n");
    console.log(options.prompt);
    console.log("\n--- end prompt ---");
    return null;
  }

  const repoRoot = resolveRepoRoot();
  const agent = await Agent.create(buildAgentOptions(repoRoot));

  try {
    const run = await agent.send(options.prompt);
    console.error(`agentId=${agent.agentId} runId=${run.id}`);

    if (run.supports("stream")) {
      for await (const event of run.stream()) {
        if (event.type === "assistant") {
          for (const block of event.message.content) {
            if (block.type === "text") {
              const text = block.text;
              if (options.onStdout) {
                options.onStdout(text);
              } else {
                process.stdout.write(text);
              }
            }
          }
        }
      }
    }

    const result = await run.wait();
    if (result.status === "error") {
      console.error(`run failed: runId=${run.id} agentId=${agent.agentId}`);
      process.exitCode = 2;
    }

    return {
      agentId: agent.agentId,
      runId: run.id,
      status: result.status,
    };
  } catch (err) {
    if (err instanceof CursorAgentError) {
      console.error(
        `startup failed: ${err.message} retryable=${err.isRetryable}`,
      );
      process.exitCode = 1;
      return null;
    }
    throw err;
  } finally {
    await agent[Symbol.asyncDispose]();
  }
}

export async function resumeLocalAgent(
  agentId: string,
  prompt: string,
  dryRun?: boolean,
): Promise<LocalRunResult | null> {
  if (dryRun) {
    console.log(`--- dry-run resume agentId=${agentId} ---\n`);
    console.log(prompt);
    console.log("\n--- end prompt ---");
    return null;
  }

  const repoRoot = resolveRepoRoot();
  const agent = await Agent.resume(agentId, buildAgentOptions(repoRoot));

  try {
    const run = await agent.send(prompt);
    console.error(`agentId=${agent.agentId} runId=${run.id}`);

    if (run.supports("stream")) {
      for await (const event of run.stream()) {
        if (event.type === "assistant") {
          for (const block of event.message.content) {
            if (block.type === "text") {
              process.stdout.write(block.text);
            }
          }
        }
      }
    }

    const result = await run.wait();
    if (result.status === "error") {
      console.error(`run failed: runId=${run.id}`);
      process.exitCode = 2;
    }

    return {
      agentId: agent.agentId,
      runId: run.id,
      status: result.status,
    };
  } catch (err) {
    if (err instanceof CursorAgentError) {
      console.error(
        `startup failed: ${err.message} retryable=${err.isRetryable}`,
      );
      process.exitCode = 1;
      return null;
    }
    throw err;
  } finally {
    await agent[Symbol.asyncDispose]();
  }
}
