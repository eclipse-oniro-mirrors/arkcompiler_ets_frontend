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

import { createArtifactFactory, Pipeline, Stage } from '../../src/pipeline';

interface TypeTestContext {
  readonly seed: number;
}

interface TypeTestArtifacts {
  readonly first: { readonly value: number };
  readonly second: { readonly value: number };
}

const artifact = createArtifactFactory<TypeTestArtifacts>();
const FIRST = artifact('first');
const SECOND = artifact('second');

const provideFirst = Stage.start<TypeTestContext>('provide-first')
  .use('provide', {
    inputs: [],
    run: (scope) => ({ value: scope.context.seed }),
  })
  .provides(FIRST, {
    build: (_scope, outputs) => outputs.provide,
  });

const consumeFirst = Stage.start<TypeTestContext>('consume-first')
  .requires(FIRST)
  .use('consume', {
    inputs: [],
    run: (scope) => scope.get(FIRST).value,
  });

Pipeline.start<TypeTestContext>().stage(provideFirst).stage(consumeFirst);

Stage.start<TypeTestContext>('combine')
  .use('left', {
    inputs: [],
    run: (scope) => scope.context.seed,
  })
  .use('right', {
    inputs: [],
    run: (scope) => scope.context.seed + 1,
  })
  .provides(SECOND, {
    build: (_scope, outputs) => ({ value: outputs.left + outputs.right }),
  });

function assertRejectedPipelineShapes(): void {
  assertRejectedPipelineAssembly();
  assertRejectedStageReads();
  assertRejectedHookDefinitions();
  assertRejectedProvidesPayloads();
  assertRejectedProvidesInterface();
}

function assertRejectedPipelineAssembly(): void {
  // @ts-expect-error FIRST must be provided before a stage can require it.
  Pipeline.start<TypeTestContext>().stage(consumeFirst);

  // @ts-expect-error A named artifact cannot be provided twice.
  Pipeline.start<TypeTestContext>().stage(provideFirst).stage(provideFirst);
}

function assertRejectedStageReads(): void {
  Stage.start<TypeTestContext>('undeclared-artifact-read')
    .requires(FIRST)
    .use('read', {
      inputs: [],
      run: (scope) => {
        // @ts-expect-error SECOND was not declared in stage requires.
        return scope.get(SECOND);
      },
    });

  Stage.start<TypeTestContext>('undeclared-hook-read')
    .use('first', {
      inputs: [],
      run: () => 1,
    })
    .use('second', {
      inputs: [],
      run: (_scope, inputs) => {
        // @ts-expect-error `first` was not declared in this hook's inputs.
        return inputs.first;
      },
    });
}

function assertRejectedHookDefinitions(): void {
  Stage.start<TypeTestContext>('unknown-hook-input').use('read', {
    // @ts-expect-error A hook can only input outputs from earlier hooks.
    inputs: ['missing'],
    run: () => undefined,
  });

  const firstHook = Stage.start<TypeTestContext>('duplicate-hook').use('same-name', {
    inputs: [],
    run: () => 1,
  });
  firstHook.use(
    // @ts-expect-error Hook names must be unique inside a stage.
    'same-name',
    {
      inputs: [],
      run: () => 2,
    },
  );

  Stage.start<TypeTestContext>('chain-defines-order')
    .use('first', {
      inputs: [],
      run: () => 1,
    })
    .use('second', {
      inputs: [],
      // @ts-expect-error Chained use calls define order; `after` is not part of the interface.
      after: ['first'],
      run: () => 2,
    });
}

function assertRejectedProvidesPayloads(): void {
  const wrongPayload = Stage.start<TypeTestContext>('wrong-payload').use('produce', {
    inputs: [],
    run: () => ({ value: 'wrong' }),
  });
  wrongPayload.provides(FIRST, {
    // @ts-expect-error The explicit stage result must satisfy FIRST's value type.
    build: (_scope, outputs) => outputs.produce,
  });

  const completeBuildOutputs = Stage.start<TypeTestContext>('complete-build-outputs')
    .use('first', {
      inputs: [],
      run: () => 1,
    })
    .use('second', {
      inputs: [],
      run: () => 2,
    });
  completeBuildOutputs.provides(SECOND, {
    build: (_scope, outputs) => ({
      value: outputs.first + outputs.second,
    }),
  });
}

function assertRejectedProvidesInterface(): void {
  const unknownBuildOutput = Stage.start<TypeTestContext>('unknown-build-output').use('known', {
    inputs: [],
    run: () => 1,
  });
  unknownBuildOutput.provides(SECOND, {
    build: (_scope, outputs) => ({
      // @ts-expect-error build only exposes outputs from hooks declared in this stage.
      value: outputs.missing,
    }),
  });

  const removedProvidesInputs = Stage.start<TypeTestContext>('removed-provides-inputs').use('produce', {
    inputs: [],
    run: () => ({ value: 1 }),
  });
  removedProvidesInputs.provides(FIRST, {
    // @ts-expect-error provides no longer accepts an inputs filter.
    inputs: ['produce'],
    build: (_scope, outputs) => outputs.produce,
  });
}

void assertRejectedPipelineShapes;
