/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

import { type AnyArtifact, type ArtifactList, type ArtifactRecord, type ArtifactValue } from './artifact';
import { type HookStage, ProvidedStage, type StageScope } from './stage';

interface PipelineStep<Context> {
  readonly name: string;
  readonly requires: ArtifactList;
  readonly provides?: AnyArtifact;
  readonly invoke: (context: Readonly<Context>, store: ReadonlyMap<string, unknown>) => Promise<unknown>;
}

type MissingRequirements<Available extends object, Requirements extends ArtifactList> = {
  readonly missingArtifacts: Exclude<keyof ArtifactRecord<Requirements>, keyof Available>;
};

type DuplicateArtifact<Available extends object, Provided extends AnyArtifact> = {
  readonly duplicateArtifact: Extract<Provided['name'], keyof Available>;
};

type AvailableRequirement<Available extends object, Requirements extends ArtifactList> =
  Available extends ArtifactRecord<Requirements> ? unknown : MissingRequirements<Available, Requirements>;

type NewArtifact<Available extends object, Provided extends AnyArtifact> = Provided['name'] extends keyof Available
  ? DuplicateArtifact<Available, Provided>
  : unknown;

/**
 * An immutable stage chain. Execution order is chained, while data dependencies
 * are selected from the per-run artifact store through requires/provides.
 */
export class Pipeline<Context, Available extends object, Output> {
  public readonly stageNames: readonly string[];

  private constructor(private readonly steps: readonly PipelineStep<Context>[]) {
    this.stageNames = steps.map((step) => step.name);
  }

  public static start<Context>(): Pipeline<Context, {}, void> {
    return new Pipeline<Context, {}, void>([]);
  }

  public stage<Requirements extends ArtifactList, HookOutputs extends object>(
    stage: HookStage<Context, Requirements, HookOutputs> & AvailableRequirement<Available, Requirements>,
  ): Pipeline<Context, Available, void>;

  public stage<Requirements extends ArtifactList, Provided extends AnyArtifact>(
    stage: ProvidedStage<Context, Requirements, Provided> &
      AvailableRequirement<Available, Requirements> &
      NewArtifact<Available, Provided>,
  ): Pipeline<
    Context,
    Available & Readonly<Record<Provided['name'], ArtifactValue<Provided>>>,
    ArtifactValue<Provided>
  >;

  public stage<Requirements extends ArtifactList, HookOutputs extends object, Provided extends AnyArtifact>(
    stage: HookStage<Context, Requirements, HookOutputs> | ProvidedStage<Context, Requirements, Provided>,
  ): unknown {
    const availableNames = new Set(
      this.steps.flatMap((step) => (step.provides === undefined ? [] : [step.provides.name])),
    );
    for (const requirement of stage.requires) {
      if (!availableNames.has(requirement.name)) {
        throw new Error(`stage ${stage.name} requires unavailable artifact: ${requirement.name}`);
      }
    }
    const providedArtifact = stage instanceof ProvidedStage ? stage.provides : undefined;
    if (providedArtifact !== undefined && availableNames.has(providedArtifact.name)) {
      throw new Error(`pipeline artifact is provided more than once: ${providedArtifact.name}`);
    }

    const step = createPipelineStep(stage, providedArtifact);
    return new Pipeline([...this.steps, step]);
  }

  public async run(context: Readonly<Context>): Promise<Output> {
    const store = new Map<string, unknown>();
    const remainingConsumers = countArtifactConsumers(this.steps);
    let output: unknown = undefined;

    for (const step of this.steps) {
      output = await step.invoke(context, store);
      if (step.provides === undefined) {
        output = undefined;
      } else {
        store.set(step.provides.name, output);
      }

      releaseConsumedArtifacts(step.requires, remainingConsumers, store);
      if (step.provides !== undefined && (remainingConsumers.get(step.provides.name) ?? 0) === 0) {
        store.delete(step.provides.name);
      }
    }

    return output as Output;
  }
}

function createPipelineStep<
  Context,
  Requirements extends ArtifactList,
  HookOutputs extends object,
  Provided extends AnyArtifact,
>(
  stage: HookStage<Context, Requirements, HookOutputs> | ProvidedStage<Context, Requirements, Provided>,
  providedArtifact: Provided | undefined,
): PipelineStep<Context> {
  return {
    name: stage.name,
    requires: stage.requires,
    invoke: async (context, store): Promise<unknown> => stage.run(createStageScope(context, stage.requires, store)),
    ...(providedArtifact === undefined ? {} : { provides: providedArtifact }),
  };
}

function createStageScope<Context, Requirements extends ArtifactList>(
  context: Readonly<Context>,
  requirements: Requirements,
  store: ReadonlyMap<string, unknown>,
): StageScope<Context, Requirements> {
  const allowedNames = new Set(requirements.map((artifact) => artifact.name));
  return {
    context,
    get<Current extends Requirements[number]>(artifact: Current): ArtifactValue<Current> {
      if (!allowedNames.has(artifact.name)) {
        throw new Error(`stage did not declare required artifact: ${artifact.name}`);
      }
      if (!store.has(artifact.name)) {
        throw new Error(`required pipeline artifact is unavailable: ${artifact.name}`);
      }
      return store.get(artifact.name) as ArtifactValue<Current>;
    },
  };
}

function countArtifactConsumers<Context>(steps: readonly PipelineStep<Context>[]): Map<string, number> {
  const consumers = new Map<string, number>();
  for (const step of steps) {
    for (const artifact of step.requires) {
      consumers.set(artifact.name, (consumers.get(artifact.name) ?? 0) + 1);
    }
  }
  return consumers;
}

function releaseConsumedArtifacts(
  artifacts: ArtifactList,
  remainingConsumers: Map<string, number>,
  store: Map<string, unknown>,
): void {
  for (const artifact of artifacts) {
    const remaining = (remainingConsumers.get(artifact.name) ?? 0) - 1;
    remainingConsumers.set(artifact.name, remaining);
    if (remaining === 0) {
      store.delete(artifact.name);
    }
  }
}
