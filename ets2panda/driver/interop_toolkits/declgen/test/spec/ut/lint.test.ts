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

import { loadSpec } from '../framework/src/loader';
import { lintSpec } from '../framework/src/lint';
import { makeSpecFixture, writeCase } from './helpers';

function errorsOf(specDir: string): string[] {
  return lintSpec(loadSpec(specDir))
    .filter((issue) => issue.severity === 'error')
    .map((issue) => `${issue.relPath}: ${issue.message}`);
}

describe('lint: document rules', () => {
  it('accepts the well-formed fixture', () => {
    const fx = makeSpecFixture();
    expect(errorsOf(fx.specDir)).toEqual([]);
  });

  it('flags missing front-matter titles', () => {
    const fx = makeSpecFixture();
    fx.write('sections/dyn2sta/ambient/case-a/desc.zh.md', '---\n---\n\n无标题\n');
    expect(errorsOf(fx.specDir).some((m) => m.includes('title is empty'))).toBe(true);
  });

  it('flags fence count mismatch between languages', () => {
    const fx = makeSpecFixture();
    fx.write(
      'sections/dyn2sta/ambient/case-a/desc.zh.md',
      '---\ntitle: 场景A\n---\n\n描述\n\n```typescript\nconst a = 1;\n```\n',
    );
    fx.write('sections/dyn2sta/ambient/case-a/desc.en.md', '---\ntitle: Scenario A\n---\n\nDescription\n');
    expect(errorsOf(fx.specDir).some((m) => m.includes('fence count mismatch'))).toBe(true);
  });

  it('flags misplaced scenario order (incompatible before fully-compatible)', () => {
    const fx = makeSpecFixture();
    writeCase(fx.sectionsDir, 'dyn2sta/ambient/zz-incompatible', '场景C', 'Scenario C', 'incompatible', false);
    writeCase(fx.sectionsDir, 'dyn2sta/ambient/zz-compatible', '场景D', 'Scenario D', 'fully-compatible', false);
    fx.write(
      'SUMMARY.md',
      '# Spec summary\n\n- dyn2sta\n  - ambient\n    - case-a\n    - zz-incompatible\n    - zz-compatible\n- sta2dyn\n  - ambient\n    - case-b\n',
    );
    expect(errorsOf(fx.specDir).some((m) => m.includes('compat order'))).toBe(true);
  });

  it('flags topic title mismatch across directions', () => {
    const fx = makeSpecFixture();
    fx.write('sections/sta2dyn/ambient/section.zh.md', '---\ntitle: 另一个标题\n---\n\n**介绍**\n');
    expect(errorsOf(fx.specDir).some((m) => m.includes('title mismatch'))).toBe(true);
  });

  it('flags unknown anchor references', () => {
    const fx = makeSpecFixture();
    fx.write('sections/dyn2sta/ambient/case-a/desc.zh.md', '---\ntitle: 场景A\n---\n\n参见{#no-such-anchor}\n');
    expect(errorsOf(fx.specDir).some((m) => m.includes('unknown anchor'))).toBe(true);
  });
});

describe('lint: placeholder rules', () => {
  it('flags {{sample:input}} without the sample file', () => {
    const fx = makeSpecFixture();
    fx.write('sections/dyn2sta/ambient/case-a/desc.zh.md', '---\ntitle: 场景A\n---\n\n输入：\n\n{{sample:input}}\n');
    expect(errorsOf(fx.specDir).some((m) => m.includes('sample.input.ets is missing'))).toBe(true);
  });

  it('downgrades missing sample output to a warning in blessing mode', () => {
    const fx = makeSpecFixture();
    fx.write('sections/dyn2sta/ambient/case-a/desc.zh.md', '---\ntitle: 场景A\n---\n\n输出：\n\n{{sample:output}}\n');
    const issues = lintSpec(loadSpec(fx.specDir), { blessing: true });
    const missing = issues.filter((issue) => issue.message.includes('sample.output.d.ets'));
    expect(missing).toHaveLength(1);
    expect(missing[0]!.severity).toBe('warning');
    expect(lintSpec(loadSpec(fx.specDir)).filter((i) => i.severity === 'error').length).toBeGreaterThan(0);
  });
});
