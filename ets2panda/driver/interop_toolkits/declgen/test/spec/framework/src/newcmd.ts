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
 * `spec-kit new <parent/path> <slug>` scaffolds a new case directory and
 * appends it to SUMMARY.md (at the end of the parent's children). The
 * scaffold ships placeholder sample input only; sample output and fixture
 * goldens are produced by `spec-kit test --update` and must be reviewed
 * before committing.
 */

import * as fs from 'fs';
import * as path from 'path';

import { parseSummary, serializeSummary, summaryPathOf } from './summary';

const sampleInputTemplate = `// TODO: add the sample input for this scenario
`;

const descZhTemplate = `---
title: TODO：场景标题（中文）
---

TODO：场景描述（中文）。

输入：

{{sample:input}}

输出：

{{sample:output}}
`;

const descEnTemplate = `---
title: TODO: scenario title (English)
---

TODO: scenario description (English).

Input:

{{sample:input}}

Output:

{{sample:output}}
`;

export function scaffoldCase(specDir: string, parentPath: string, slug: string): string {
  const sectionsDir = path.join(specDir, 'sections');
  const parentDir = path.join(sectionsDir, ...parentPath.split('/'));
  if (!fs.existsSync(parentDir)) {
    throw new Error(`parent directory does not exist: ${parentDir}`);
  }
  if (fs.existsSync(path.join(parentDir, 'case.json'))) {
    throw new Error(`parent is a case, not a section: ${parentPath}`);
  }
  if (!/^[a-z0-9][a-z0-9-]*$/.test(slug)) {
    throw new Error(`slug must be lowercase kebab-case: ${slug}`);
  }
  const relPath = `${parentPath}/${slug}`;
  const caseDir = path.join(parentDir, slug);
  if (fs.existsSync(caseDir)) {
    throw new Error(`case directory already exists: ${relPath}`);
  }

  fs.mkdirSync(path.join(caseDir, 'input'), { recursive: true });
  fs.writeFileSync(
    path.join(caseDir, 'case.json'),
    `${JSON.stringify({ compat: 'requires-transform', tags: [] }, null, 2)}\n`,
  );
  fs.writeFileSync(path.join(caseDir, 'input', 'main.ts'), sampleInputTemplate);
  fs.writeFileSync(path.join(caseDir, 'sample.input.ets'), sampleInputTemplate);
  fs.writeFileSync(path.join(caseDir, 'desc.zh.md'), descZhTemplate);
  fs.writeFileSync(path.join(caseDir, 'desc.en.md'), descEnTemplate);

  const summaryPath = summaryPathOf(sectionsDir);
  const { doc } = parseSummary(fs.readFileSync(summaryPath, 'utf-8'));
  const depth = parentPath.split('/').length;
  // Keep entries grouped: insert before the next entry with depth <= parent depth.
  const entry = { relPath, depth };
  let insertAt = doc.entries.length;
  let seenParent = false;
  for (let i = 0; i < doc.entries.length; i++) {
    const current = doc.entries[i]!;
    if (current.relPath.startsWith(`${parentPath}/`)) {
      seenParent = true;
      continue;
    }
    if (seenParent && current.depth <= depth - 1) {
      insertAt = i;
      break;
    }
  }
  doc.entries.splice(insertAt, 0, entry);
  fs.writeFileSync(summaryPath, serializeSummary(doc));
  return relPath;
}
