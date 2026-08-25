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

import { type Artifact, type ArtifactList, type ArtifactValue } from './artifact';

type HookOutputName<Outputs extends object> = Extract<keyof Outputs, string>;

type SelectedHookOutputs<Outputs extends object, Names extends readonly HookOutputName<Outputs>[]> = Readonly<{
  [Name in Names[number]]: Outputs[Name];
}>;

type NewHookName<Outputs extends object, Name extends string> = Name extends keyof Outputs
  ? { readonly duplicateHookName: Name }
  : unknown;

/** The only run-wide and cross-stage artifact access visible to a hook. */
export interface StageScope<Context, Requirements extends ArtifactList> {
  readonly context: Readonly<Context>;

  get<Current extends Requirements[number]>(artifact: Current): ArtifactValue<Current>;
}

export interface HookDefinition<
  Context,
  Requirements extends ArtifactList,
  Outputs extends object,
  Inputs extends readonly HookOutputName<Outputs>[],
  HookOutput,
> {
  /** Hook outputs whose values are passed to `run`. */
  readonly inputs: Inputs;
  readonly run: (scope: StageScope<Context, Requirements>, inputs: SelectedHookOutputs<Outputs, Inputs>) => HookOutput;
}

export interface StageResultDefinition<Context, Requirements extends ArtifactList, Outputs extends object, Result> {
  readonly build: (scope: StageScope<Context, Requirements>, outputs: Readonly<Outputs>) => Result | Promise<Result>;
}

export interface StageStart<Context> {
  readonly name: string;
  readonly hookNames: readonly string[];

  requires<Requirements extends ArtifactList>(...artifacts: Requirements): RequiredStageStart<Context, Requirements>;

  use<Name extends string, Inputs extends readonly never[], HookOutput>(
    stableName: Name,
    definition: HookDefinition<Context, readonly [], {}, Inputs, HookOutput>,
  ): HookStage<Context, readonly [], Readonly<Record<Name, Awaited<HookOutput>>>>;
}

export interface RequiredStageStart<Context, Requirements extends ArtifactList> {
  readonly name: string;
  readonly hookNames: readonly string[];
  readonly requirements: Requirements;

  use<Name extends string, Inputs extends readonly never[], HookOutput>(
    stableName: Name,
    definition: HookDefinition<Context, Requirements, {}, Inputs, HookOutput>,
  ): HookStage<Context, Requirements, Readonly<Record<Name, Awaited<HookOutput>>>>;
}

interface HookStep<Context, Requirements extends ArtifactList> {
  readonly stableName: string;
  readonly inputs: readonly string[];
  readonly invoke: (
    scope: StageScope<Context, Requirements>,
    inputs: Readonly<Record<string, unknown>>,
  ) => Promise<unknown>;
}

/** An immutable builder for named hook outputs inside one stage. */
export class HookStage<Context, Requirements extends ArtifactList, Outputs extends object> {
  public readonly name: string;
  public readonly hookNames: readonly string[];
  public readonly requires: Requirements;

  public constructor(
    name: string,
    requirements: Requirements,
    private readonly steps: readonly HookStep<Context, Requirements>[],
  ) {
    this.name = name;
    this.requires = requirements;
    this.hookNames = steps.map((step) => step.stableName);
  }

  public use<Name extends string, Inputs extends readonly HookOutputName<Outputs>[], HookOutput>(
    stableName: Name & NewHookName<Outputs, Name>,
    definition: HookDefinition<Context, Requirements, Outputs, Inputs, HookOutput>,
  ): HookStage<Context, Requirements, Outputs & Readonly<Record<Name, Awaited<HookOutput>>>> {
    assertHookDefinition(stableName, definition.inputs, this.hookNames);
    return new HookStage(this.name, this.requires, [...this.steps, createHookStep(stableName, definition)]);
  }

  public async run(scope: StageScope<Context, Requirements>): Promise<void> {
    await this.runHooks(scope);
  }

  public provides<Name extends string, Value>(
    artifact: Artifact<Name, Value>,
    definition: StageResultDefinition<Context, Requirements, Outputs, Value>,
  ): ProvidedStage<Context, Requirements, Artifact<Name, Value>> {
    return new ProvidedStage(
      this.name,
      this.hookNames,
      this.requires,
      artifact,
      async (scope): Promise<ArtifactValue<Artifact<Name, Value>>> => {
        const outputs = await this.runHooks(scope);
        const result = await definition.build(scope, outputs);
        return result as ArtifactValue<Artifact<Name, Value>>;
      },
    );
  }

  private async runHooks(scope: StageScope<Context, Requirements>): Promise<Readonly<Outputs>> {
    const outputs: Record<string, unknown> = Object.create(null) as Record<string, unknown>;
    for (const step of this.steps) {
      assertKnownNames(`hook ${step.stableName} inputs`, step.inputs, Object.keys(outputs));
      outputs[step.stableName] = await step.invoke(scope, selectHookOutputs(outputs, step.inputs));
    }
    return Object.freeze(outputs) as Readonly<Outputs>;
  }
}

/** A completed stage definition that can be registered with a Pipeline. */
export class ProvidedStage<Context, Requirements extends ArtifactList, Provided extends Artifact<string, unknown>> {
  public constructor(
    public readonly name: string,
    public readonly hookNames: readonly string[],
    public readonly requires: Requirements,
    public readonly provides: Provided,
    public readonly run: (scope: StageScope<Context, Requirements>) => Promise<ArtifactValue<Provided>>,
  ) {}
}

/** Starts an internal stage definition. */
export class Stage {
  private constructor() {}

  public static start<Context>(name: string): StageStart<Context> {
    return new StageStartBuilder<Context>(name);
  }
}

class StageStartBuilder<Context> implements StageStart<Context> {
  public readonly hookNames: readonly string[] = [];

  public constructor(public readonly name: string) {}

  public requires<Requirements extends ArtifactList>(
    ...artifacts: Requirements
  ): RequiredStageStart<Context, Requirements> {
    assertUniqueNames(
      artifacts.map((artifact) => artifact.name),
      `stage ${this.name} requirements`,
    );
    return new RequiredStageStartBuilder(this.name, artifacts);
  }

  public use<Name extends string, Inputs extends readonly never[], HookOutput>(
    stableName: Name,
    definition: HookDefinition<Context, readonly [], {}, Inputs, HookOutput>,
  ): HookStage<Context, readonly [], Readonly<Record<Name, Awaited<HookOutput>>>> {
    assertHookDefinition(stableName, definition.inputs, []);
    return new HookStage(this.name, [], [createHookStep(stableName, definition)]);
  }
}

class RequiredStageStartBuilder<Context, Requirements extends ArtifactList>
  implements RequiredStageStart<Context, Requirements>
{
  public readonly hookNames: readonly string[] = [];

  public constructor(
    public readonly name: string,
    public readonly requirements: Requirements,
  ) {}

  public use<Name extends string, Inputs extends readonly never[], HookOutput>(
    stableName: Name,
    definition: HookDefinition<Context, Requirements, {}, Inputs, HookOutput>,
  ): HookStage<Context, Requirements, Readonly<Record<Name, Awaited<HookOutput>>>> {
    assertHookDefinition(stableName, definition.inputs, []);
    return new HookStage(this.name, this.requirements, [createHookStep(stableName, definition)]);
  }
}

function createHookStep<
  Context,
  Requirements extends ArtifactList,
  Outputs extends object,
  Inputs extends readonly HookOutputName<Outputs>[],
  HookOutput,
>(
  stableName: string,
  definition: HookDefinition<Context, Requirements, Outputs, Inputs, HookOutput>,
): HookStep<Context, Requirements> {
  return {
    stableName,
    inputs: definition.inputs,
    invoke: async (scope, inputs): Promise<unknown> =>
      definition.run(scope, inputs as SelectedHookOutputs<Outputs, Inputs>),
  };
}

function selectHookOutputs(
  outputs: Readonly<Record<string, unknown>>,
  names: readonly string[],
): Readonly<Record<string, unknown>> {
  const selected: Record<string, unknown> = Object.create(null) as Record<string, unknown>;
  for (const name of names) {
    if (!Object.prototype.hasOwnProperty.call(outputs, name)) {
      throw new Error(`required hook output is unavailable: ${name}`);
    }
    selected[name] = outputs[name];
  }
  return Object.freeze(selected);
}

function assertHookDefinition(stableName: string, inputs: readonly string[], availableNames: readonly string[]): void {
  if (stableName.trim() === '') {
    throw new Error('hook name must be a non-empty string');
  }
  if (availableNames.includes(stableName)) {
    throw new Error(`stage hook is registered more than once: ${stableName}`);
  }
  assertKnownNames(`hook ${stableName} inputs`, inputs, availableNames);
  assertUniqueNames(inputs, `hook ${stableName} inputs`);
}

function assertKnownNames(owner: string, names: readonly string[], availableNames: readonly string[]): void {
  const available = new Set(availableNames);
  for (const name of names) {
    if (!available.has(name)) {
      throw new Error(`${owner} references unavailable hook: ${name}`);
    }
  }
}

function assertUniqueNames(names: readonly string[], owner: string): void {
  const seen = new Set<string>();
  for (const name of names) {
    if (seen.has(name)) {
      throw new Error(`${owner} contains duplicate name: ${name}`);
    }
    seen.add(name);
  }
}
