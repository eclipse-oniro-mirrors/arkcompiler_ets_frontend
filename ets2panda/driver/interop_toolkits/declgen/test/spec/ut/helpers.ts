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

/** Shared fixture factory: builds a minimal but valid spec suite in a temp dir. */

import * as fs from 'fs';
import * as os from 'os';
import * as path from 'path';

export interface Fixture {
  specDir: string;
  sectionsDir: string;
  summaryPath: string;
  write(relPath: string, content: string): void;
  remove(relPath: string): void;
}

const SUMMARY = `# Spec summary

- dyn2sta
  - ambient
    - case-a
- sta2dyn
  - ambient
    - case-b
`;

const CONFIG = JSON.stringify({
  docId: 'test-spec',
  title: { zh: '测试规格', en: 'Test Spec' },
  frontMatter: [],
  structure: 'topic-major',
});

function writeSection(
  sectionsDir: string,
  rel: string,
  zhTitle: string,
  enTitle: string,
  zhBody = '',
  enBody = '',
): void {
  const dir = path.join(sectionsDir, ...rel.split('/'));
  fs.mkdirSync(dir, { recursive: true });
  fs.writeFileSync(path.join(dir, 'section.zh.md'), `---\ntitle: ${zhTitle}\n---\n\n${zhBody}`);
  fs.writeFileSync(path.join(dir, 'section.en.md'), `---\ntitle: ${enTitle}\n---\n\n${enBody}`);
}

function writeDirections(sectionsDir: string): void {
  for (const direction of ['dyn2sta', 'sta2dyn']) {
    const dir = path.join(sectionsDir, direction);
    fs.mkdirSync(dir, { recursive: true });
    fs.writeFileSync(path.join(dir, 'node.json'), '{"kind": "direction"}\n');
  }
  writeSection(sectionsDir, 'dyn2sta', 'ArkTS-Dyn映射到ArkTS-Sta上下文', 'Mapping from ArkTS-Dyn to the ArkTS-Sta Context');
  writeSection(sectionsDir, 'sta2dyn', 'ArkTS-Sta映射到ArkTS-Dyn上下文', 'Mapping from ArkTS-Sta to the ArkTS-Dyn Context');
}

function writeTopics(sectionsDir: string): void {
  // Same topic titles across directions (lint requirement).
  writeSection(
    sectionsDir,
    'dyn2sta/ambient',
    'Ambient Declaration 规格',
    'Ambient Declaration Specification',
    '**背景**',
    '**Background**',
  );
  writeSection(
    sectionsDir,
    'sta2dyn/ambient',
    'Ambient Declaration 规格',
    'Ambient Declaration Specification',
    '**介绍**',
    '**Introduction**',
  );
}

export function makeSpecFixture(): Fixture {
  const specDir = fs.mkdtempSync(path.join(os.tmpdir(), 'spec-kit-test-'));
  const sectionsDir = path.join(specDir, 'sections');
  fs.mkdirSync(sectionsDir, { recursive: true });
  fs.writeFileSync(path.join(specDir, 'spec.config.json'), `${CONFIG}\n`);
  fs.writeFileSync(path.join(specDir, 'SUMMARY.md'), SUMMARY);
  writeDirections(sectionsDir);
  writeTopics(sectionsDir);

  writeCase(sectionsDir, 'dyn2sta/ambient/case-a', '场景A', 'Scenario A', 'requires-transform', false);
  writeCase(sectionsDir, 'sta2dyn/ambient/case-b', '场景B', 'Scenario B', 'requires-transform', true);

  const fixture: Fixture = {
    specDir,
    sectionsDir,
    summaryPath: path.join(specDir, 'SUMMARY.md'),
    write(relPath, content) {
      const target = path.join(specDir, ...relPath.split('/'));
      fs.mkdirSync(path.dirname(target), { recursive: true });
      fs.writeFileSync(target, content);
    },
    remove(relPath) {
      fs.rmSync(path.join(specDir, ...relPath.split('/')), { recursive: true, force: true });
    },
  };
  return fixture;
}

export function writeCase(
  sectionsDir: string,
  rel: string,
  zhTitle: string,
  enTitle: string,
  compat: string,
  pending: boolean,
  zhBody = '描述A',
  enBody = 'Description A',
): void {
  const dir = path.join(sectionsDir, ...rel.split('/'));
  fs.mkdirSync(path.join(dir, 'input'), { recursive: true });
  fs.writeFileSync(path.join(dir, 'case.json'), `${JSON.stringify({ compat, tags: [], pending }, null, 2)}\n`);
  fs.writeFileSync(path.join(dir, 'input', 'main.ts'), 'export const a: number = 1;\n');
  fs.writeFileSync(path.join(dir, 'desc.zh.md'), `---\ntitle: ${zhTitle}\n---\n\n${zhBody}\n`);
  fs.writeFileSync(path.join(dir, 'desc.en.md'), `---\ntitle: ${enTitle}\n---\n\n${enBody}\n`);
}
