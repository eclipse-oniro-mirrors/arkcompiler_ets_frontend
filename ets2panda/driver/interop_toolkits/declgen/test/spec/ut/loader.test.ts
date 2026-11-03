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

import { LoadError, loadSpec } from '../framework/src/loader';
import { isCase, isSection } from '../framework/src/model';
import { makeSpecFixture, writeCase } from './helpers';

describe('loader', () => {
  it('loads a valid suite in SUMMARY order', () => {
    const fx = makeSpecFixture();
    const model = loadSpec(fx.specDir);
    expect(model.roots.map((n) => n.slug)).toEqual(['dyn2sta', 'sta2dyn']);
    const dyn2sta = model.roots[0];
    expect(isSection(dyn2sta) && dyn2sta.direction).toBe(true);
    expect(model.cases.map((c) => c.relPath)).toEqual(['dyn2sta/ambient/case-a', 'sta2dyn/ambient/case-b']);
    expect(model.cases.map((c) => c.pending)).toEqual([false, true]);
    expect(model.anchors.has('dyn2sta-ambient-case-a')).toBe(true);
    expect(isCase(model.cases[0]!)).toBe(true);
  });

  it('rejects directories missing from SUMMARY.md', () => {
    const fx = makeSpecFixture();
    writeCase(fx.sectionsDir, 'dyn2sta/ambient/orphan', '孤儿', 'Orphan', 'requires-transform', false);
    expect(() => loadSpec(fx.specDir)).toThrowError(LoadError);
  });

  it('rejects SUMMARY entries without a directory', () => {
    const fx = makeSpecFixture();
    fx.write(
      'SUMMARY.md',
      '# Spec summary\n\n- dyn2sta\n  - ambient\n    - case-a\n    - ghost\n- sta2dyn\n  - ambient\n    - case-b\n',
    );
    expect(() => loadSpec(fx.specDir)).toThrowError(/ghost/);
  });

  it('rejects direction groups below root level', () => {
    const fx = makeSpecFixture();
    fx.write('sections/dyn2sta/ambient/nested-direction/node.json', '{"kind": "direction"}\n');
    fx.write('sections/dyn2sta/ambient/nested-direction/section.zh.md', '---\ntitle: t\n---\n');
    fx.write('sections/dyn2sta/ambient/nested-direction/section.en.md', '---\ntitle: t\n---\n');
    fx.write(
      'SUMMARY.md',
      '# Spec summary\n\n- dyn2sta\n  - ambient\n    - case-a\n    - nested-direction\n- sta2dyn\n  - ambient\n    - case-b\n',
    );
    expect(() => loadSpec(fx.specDir)).toThrowError(/direction group/);
  });

  it('rejects invalid compat values', () => {
    const fx = makeSpecFixture();
    fx.write('sections/dyn2sta/ambient/case-a/case.json', '{"compat": "sometimes"}\n');
    expect(() => loadSpec(fx.specDir)).toThrowError(/invalid compat/);
  });
});
