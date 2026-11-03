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

import { parseSummary, serializeSummary } from '../framework/src/summary';

describe('summary parsing', () => {
  it('parses nested entries with relative paths', () => {
    const { doc, issues } = parseSummary('# Header\n\n- dyn2sta\n  - ambient\n    - add-declare\n- sta2dyn\n');
    expect(issues).toEqual([]);
    expect(doc.header).toBe('# Header');
    expect(doc.entries.map((e) => e.relPath)).toEqual([
      'dyn2sta',
      'dyn2sta/ambient',
      'dyn2sta/ambient/add-declare',
      'sta2dyn',
    ]);
    expect(doc.entries.map((e) => e.depth)).toEqual([0, 1, 2, 0]);
  });

  it('rejects odd indentation and skipped levels', () => {
    const { issues } = parseSummary('- a\n   - b\n- c\n        - d\n');
    expect(issues.length).toBeGreaterThan(0);
  });

  it('round-trips through serialization', () => {
    const original = '# Spec summary\n\n- dyn2sta\n  - ambient\n    - add-declare\n- sta2dyn\n';
    const { doc, issues } = parseSummary(original);
    expect(issues).toEqual([]);
    expect(serializeSummary(doc)).toBe(original);
  });
});
