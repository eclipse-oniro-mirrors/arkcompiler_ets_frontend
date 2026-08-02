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

interface RunContext {
  readonly value: number;
}

interface TestArtifactTypes {
  readonly prepared: {
    readonly value: number;
    readonly trace: readonly string[];
    readonly prepared: true;
  };
  readonly result: { readonly result: string };
}

const artifact = createArtifactFactory<TestArtifactTypes>();
const PREPARED = artifact('prepared');
const RESULT = artifact('result');

describe('Pipeline', () => {
  it('runs hooks and stages in chained registration order', async () => {
    const calls: string[] = [];
    const prepare = Stage.start<RunContext>('prepare')
      .use('normalize-input', {
        inputs: [],
        run: (scope) => {
          calls.push('normalize-input');
          return { value: scope.context.value, trace: ['normalized'] };
        },
      })
      .use('prepare-project', {
        inputs: ['normalize-input'],
        run: async (_scope, inputs) => {
          calls.push('prepare-project');
          return { ...inputs['normalize-input'], prepared: true as const };
        },
      })
      .provides(PREPARED, {
        build: (_scope, outputs) => outputs['prepare-project'],
      });
    const generate = Stage.start<RunContext>('generate')
      .requires(PREPARED)
      .use('format-result', {
        inputs: [],
        run: (scope) => {
          calls.push('format-result');
          return { result: `value=${scope.get(PREPARED).value}` };
        },
      })
      .provides(RESULT, {
        build: (_scope, outputs) => outputs['format-result'],
      });

    await expect(Pipeline.start<RunContext>().stage(prepare).stage(generate).run({ value: 7 })).resolves.toEqual({
      result: 'value=7',
    });
    expect(calls).toEqual(['normalize-input', 'prepare-project', 'format-result']);
  });

  it('supports a terminal stage without provides', async () => {
    const calls: string[] = [];
    const prepare = Stage.start<RunContext>('prepare')
      .use('prepare', {
        inputs: [],
        run: (scope) => ({
          value: scope.context.value,
          trace: [],
          prepared: true as const,
        }),
      })
      .provides(PREPARED, {
        build: (_scope, outputs) => outputs.prepare,
      });
    const terminal = Stage.start<RunContext>('terminal')
      .requires(PREPARED)
      .use('execute', {
        inputs: [],
        run: (scope) => {
          calls.push(`value=${scope.get(PREPARED).value}`);
        },
      });

    await expect(
      Pipeline.start<RunContext>().stage(prepare).stage(terminal).run({ value: 3 }),
    ).resolves.toBeUndefined();
    expect(calls).toEqual(['value=3']);
  });

  it('combines independent hook outputs without passing them between hooks', async () => {
    let secondInputs: object | undefined;
    const stage = Stage.start<RunContext>('independent-hooks')
      .use('run1', {
        inputs: [],
        run: (scope) => ({ left: scope.context.value + 1 }),
      })
      .use('run2', {
        inputs: [],
        run: (_scope, inputs) => {
          secondInputs = inputs;
          return { right: 'second' };
        },
      })
      .provides(RESULT, {
        build: (_scope, outputs) => ({
          result: `${outputs.run1.left}:${outputs.run2.right}`,
        }),
      });

    const result = await Pipeline.start<RunContext>().stage(stage).run({ value: 1 });

    expect(secondInputs).toEqual({});
    expect(result).toEqual({ result: '2:second' });
  });

  it('supports a non-adjacent dependency without adding it to an intermediate stage', async () => {
    interface DependencyArtifacts {
      readonly first: { readonly first: number };
      readonly middle: { readonly middle: number };
      readonly last: { readonly sum: number };
    }
    const dependencyArtifact = createArtifactFactory<DependencyArtifacts>();
    const FIRST = dependencyArtifact('first');
    const MIDDLE = dependencyArtifact('middle');
    const LAST = dependencyArtifact('last');
    const first = Stage.start<RunContext>('first')
      .use('produce-first', {
        inputs: [],
        run: (scope) => ({ first: scope.context.value }),
      })
      .provides(FIRST, {
        build: (_scope, outputs) => outputs['produce-first'],
      });
    const middle = Stage.start<RunContext>('middle')
      .use('produce-middle', {
        inputs: [],
        run: (scope) => ({ middle: scope.context.value * 2 }),
      })
      .provides(MIDDLE, {
        build: (_scope, outputs) => outputs['produce-middle'],
      });
    const last = Stage.start<RunContext>('last')
      .requires(FIRST, MIDDLE)
      .use('combine', {
        inputs: [],
        run: (scope) => ({
          sum: scope.get(FIRST).first + scope.get(MIDDLE).middle,
        }),
      })
      .provides(LAST, {
        build: (_scope, outputs) => outputs.combine,
      });

    await expect(
      Pipeline.start<RunContext>().stage(first).stage(middle).stage(last).run({ value: 3 }),
    ).resolves.toEqual({ sum: 9 });
    expect(middle.requires).toEqual([]);
  });

  it('propagates the original error and short-circuits later hooks and stages', async () => {
    interface FailureArtifacts {
      readonly failed: RunContext;
    }
    const FAILED = createArtifactFactory<FailureArtifacts>()('failed');
    const calls: string[] = [];
    const failure = new Error('stop');
    const failing = Stage.start<RunContext>('failing')
      .use('before-failure', {
        inputs: [],
        run: (scope) => {
          calls.push('before-failure');
          return scope.context;
        },
      })
      .use('throw-failure', {
        inputs: [],
        run: (): RunContext => {
          calls.push('throw-failure');
          throw failure;
        },
      })
      .provides(FAILED, {
        build: (_scope, outputs) => outputs['before-failure'],
      });
    const terminal = Stage.start<RunContext>('later')
      .requires(FAILED)
      .use('later-hook', {
        inputs: [],
        run: () => {
          calls.push('later-hook');
        },
      });

    await expect(Pipeline.start<RunContext>().stage(failing).stage(terminal).run({ value: 1 })).rejects.toBe(failure);
    expect(calls).toEqual(['before-failure', 'throw-failure']);
  });

  it('preserves explicit names and leaves earlier builders unchanged', () => {
    interface BuilderArtifacts {
      readonly first: RunContext;
    }
    const FIRST = createArtifactFactory<BuilderArtifacts>()('first');
    const firstStage = Stage.start<RunContext>('project.prepare').use('normalize.paths.v1', {
      inputs: [],
      run: (scope) => scope.context,
    });
    const extendedStage = firstStage.use('validate.environment.v1', {
      inputs: [],
      run: (scope) => scope.context,
    });
    const emptyPipeline = Pipeline.start<RunContext>();
    const firstPipeline = emptyPipeline.stage(
      firstStage.provides(FIRST, {
        build: (_scope, outputs) => outputs['normalize.paths.v1'],
      }),
    );
    const extendedPipeline = firstPipeline.stage(
      Stage.start<RunContext>('project.execute')
        .requires(FIRST)
        .use('native.once.v1', {
          inputs: [],
          run: () => undefined,
        }),
    );

    expect(firstStage.hookNames).toEqual(['normalize.paths.v1']);
    expect(extendedStage.hookNames).toEqual(['normalize.paths.v1', 'validate.environment.v1']);
    expect(emptyPipeline.stageNames).toEqual([]);
    expect(firstPipeline.stageNames).toEqual(['project.prepare']);
    expect(extendedPipeline.stageNames).toEqual(['project.prepare', 'project.execute']);
  });
});
