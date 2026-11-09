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
import { renderDoc } from '../framework/src/render';
import { makeSpecFixture } from './helpers';

describe('render', () => {
  it('renders topic-major with topics as H2 and directions as H3', () => {
    const fx = makeSpecFixture();
    const model = loadSpec(fx.specDir);
    const doc = renderDoc(model, 'zh', 'md', 'topic-major');
    expect(doc).toContain('## Ambient Declaration 规格');
    expect(doc).toContain('### ArkTS-Dyn映射到ArkTS-Sta上下文');
    expect(doc).toContain('### ArkTS-Sta映射到ArkTS-Dyn上下文');
    expect(doc.indexOf('## Ambient Declaration 规格')).toBeLessThan(doc.indexOf('### ArkTS-Dyn'));
  });

  it('renders direction-major with directions as H2 and topics as H3', () => {
    const fx = makeSpecFixture();
    const model = loadSpec(fx.specDir);
    const doc = renderDoc(model, 'zh', 'md', 'direction-major');
    expect(doc).toContain('## ArkTS-Dyn映射到ArkTS-Sta上下文');
    expect(doc).toContain('### Ambient Declaration 规格');
    expect(doc.indexOf('## ArkTS-Dyn')).toBeLessThan(doc.indexOf('## ArkTS-Sta'));
  });

  it('renders cases as bold titles with compat labels', () => {
    const fx = makeSpecFixture();
    const model = loadSpec(fx.specDir);
    const doc = renderDoc(model, 'zh', 'md');
    expect(doc).toContain('**场景A**');
    expect(doc).toContain('**需要转换**的场景：');
  });

  it('is byte-stable across repeated renders', () => {
    const fx = makeSpecFixture();
    const model = loadSpec(fx.specDir);
    expect(renderDoc(model, 'en', 'md')).toBe(renderDoc(model, 'en', 'md'));
  });

  it('ends with a single trailing newline', () => {
    const fx = makeSpecFixture();
    const model = loadSpec(fx.specDir);
    const doc = renderDoc(model, 'zh', 'md');
    expect(doc.endsWith('\n')).toBe(true);
    expect(doc.endsWith('\n\n')).toBe(false);
  });

  it('rejects rst until M3', () => {
    const fx = makeSpecFixture();
    const model = loadSpec(fx.specDir);
    expect(() => renderDoc(model, 'zh', 'rst')).toThrowError(/M3/);
  });
});
