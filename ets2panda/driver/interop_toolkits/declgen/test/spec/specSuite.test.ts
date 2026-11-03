/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * Jest adapter for the spec suite: one test() per case so that IDEs and CI
 * report failures per scenario. The heavy lifting lives in spec-kit
 * (test/spec/framework); this file only wires the model into Jest.
 *
 * Requires the built generator: run `npm run test:spec` (which builds first)
 * or `npm run build` manually.
 */

import * as path from 'path';

import { loadSpec, lintSpec, runCase } from './framework/src/index';

const SPEC_DIR = path.resolve(__dirname);

const model = loadSpec(SPEC_DIR);

describe('declgen spec suite', () => {
  it('lints clean', () => {
    const errors = lintSpec(model).filter((issue) => issue.severity === 'error');
    expect(errors.map((issue) => `${issue.relPath}: ${issue.message}`)).toEqual([]);
  });

  it('has at least one executable case', () => {
    expect(model.cases.some((testCase) => !testCase.pending)).toBe(true);
  });

  for (const testCase of model.cases) {
    if (testCase.pending) {
      it.skip(`${testCase.relPath} (pending)`, () => {});
      continue;
    }
    it(testCase.relPath, () => {
      const result = runCase(model, testCase);
      expect(result.messages.filter((message) => !message.startsWith('blessed'))).toEqual([]);
    });
  }
});
