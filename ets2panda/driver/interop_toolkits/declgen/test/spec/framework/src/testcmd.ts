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
 * Spec test runner. Every case is exercised twice:
 *
 *   - fixture run: the whole input/ directory goes through the dynamic
 *     Declgen generator (package_source style, rootDir = input dir) and the
 *     emitted .d.ets tree is compared against output/ (blessed golden);
 *   - sample run: sample.input.ets goes through the single-file path and the
 *     emitted declaration file is compared against sample.output.d.ets.
 *
 * Comparison semantics inherit the cookbook convention: both sides are
 * stripped of comments via the TypeScript printer before diffing, so the
 * checked-in goldens carry a license header without affecting comparison.
 * Golden bytes are only ever written by `--update` and must be reviewed by a
 * human (insta-style reviewed snapshot discipline).
 *
 * The generator is loaded lazily from the built `declgen/dist` so that the
 * lint/render/new commands keep working without a build.
 */

import * as fs from 'fs';
import * as os from 'os';
import * as path from 'path';
import { createRequire } from 'module';
import type * as tsTypes from 'typescript';

import { type CaseNode, type SpecModel } from './model';

export interface CaseResult {
  relPath: string;
  status: 'pass' | 'fail' | 'skipped' | 'blessed';
  messages: string[];
}

export interface TestRunOptions {
  update?: boolean;
  filter?: string;
}

const copyrightHeader = `/*
 * Copyright (c) ${new Date().getFullYear()} Huawei Device Co., Ltd.
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
`;

interface DeclgenBridge {
  runDyn2Sta(inputFiles: string[], rootDir: string | undefined, outDir: string): void;
  stripComments(content: string, fileName: string): string;
}

let cachedBridge: DeclgenBridge | undefined;

function declgenRootOf(specDir: string): string {
  return path.resolve(specDir, '..', '..');
}

function loadBridge(specDir: string): DeclgenBridge {
  if (cachedBridge) {
    return cachedBridge;
  }
  const declgenRoot = declgenRootOf(specDir);
  const requireFromRoot = createRequire(path.join(declgenRoot, 'package.json'));
  let tsMod: typeof tsTypes;
  try {
    tsMod = requireFromRoot('typescript') as typeof tsTypes;
  } catch (error) {
    throw new Error(`failed to load typescript from ${declgenRoot}: ${String(error)}; run "npm install" first`);
  }
  const declgenPath = path.join(declgenRoot, 'dist', 'dynamic', 'declgen.js');
  if (!fs.existsSync(declgenPath)) {
    throw new Error(`declgen build output not found: ${declgenPath}; run "npm run build" first`);
  }
  // eslint-disable-next-line @typescript-eslint/no-explicit-any
  const declgenMod: any = requireFromRoot(declgenPath);
  // eslint-disable-next-line @typescript-eslint/no-explicit-any
  const loggerMod: any = requireFromRoot(path.join(declgenRoot, 'dist', 'dynamic', 'logger', 'logger.js'));
  loggerMod.Logger.init({
    doDebug: () => {},
    doInfo: () => {},
    doWarn: () => {},
    doError: () => {},
    doTrace: () => {},
  });

  const stripComments = (content: string, fileName: string): string => {
    const sf = tsMod.createSourceFile(fileName, content, tsMod.ScriptTarget.Latest, true, tsMod.ScriptKind.TS);
    const printer = tsMod.createPrinter({ removeComments: true, newLine: tsMod.NewLineKind.LineFeed });
    return printer.printFile(sf);
  };

  const bridge: DeclgenBridge = {
    stripComments,
    runDyn2Sta(inputFiles, rootDir, outDir) {
      const declgenOptions = {
        outDir,
        rootDir,
        inputFiles,
        features: { enableInteropTypesFix: false, removeReservedKeywordIdentifier: false },
      };
      const compilerOptions = rootDir ? { baseUrl: rootDir } : undefined;
      const declgen = new declgenMod.Declgen(declgenOptions, compilerOptions);
      const result = declgen.run();
      result.emit();
    },
  };
  cachedBridge = bridge;
  return bridge;
}

function walkFiles(dir: string): string[] {
  if (!fs.existsSync(dir)) {
    return [];
  }
  const result: string[] = [];
  const walk = (current: string): void => {
    for (const entry of fs.readdirSync(current, { withFileTypes: true })) {
      const full = path.join(current, entry.name);
      if (entry.isDirectory()) {
        walk(full);
      } else {
        result.push(full);
      }
    }
  };
  walk(dir);
  return result.sort();
}

function firstDifference(expected: string, actual: string): string {
  const expectedLines = expected.split('\n');
  const actualLines = actual.split('\n');
  const max = Math.max(expectedLines.length, actualLines.length);
  for (let i = 0; i < max; i++) {
    if (expectedLines[i] !== actualLines[i]) {
      const line = i + 1;
      const exp = expectedLines[i] === undefined ? '<missing>' : expectedLines[i];
      const act = actualLines[i] === undefined ? '<missing>' : actualLines[i];
      return `first difference at line ${line}:\n    expected: ${exp}\n    actual:   ${act}`;
    }
  }
  return '';
}

/** Compare two output trees, both directions of the file set. */
function compareTrees(bridge: DeclgenBridge, expectedDir: string, actualDir: string, label: string): string[] {
  const messages: string[] = [];
  const expectedFiles = new Set(walkFiles(expectedDir).map((f) => path.relative(expectedDir, f)));
  const actualFiles = new Set(walkFiles(actualDir).map((f) => path.relative(actualDir, f)));
  for (const rel of expectedFiles) {
    if (!actualFiles.has(rel)) {
      messages.push(`${label}: missing output file: ${rel}`);
    }
  }
  for (const rel of actualFiles) {
    if (!expectedFiles.has(rel)) {
      messages.push(`${label}: unexpected output file: ${rel}`);
    }
  }
  for (const rel of expectedFiles) {
    if (!actualFiles.has(rel)) {
      continue;
    }
    const expectedPath = path.join(expectedDir, rel);
    const actualPath = path.join(actualDir, rel);
    const expected = bridge.stripComments(fs.readFileSync(expectedPath, 'utf-8'), expectedPath);
    const actual = bridge.stripComments(fs.readFileSync(actualPath, 'utf-8'), actualPath);
    if (expected !== actual) {
      messages.push(`${label}: content mismatch in ${rel}\n  ${firstDifference(expected, actual)}`);
    }
  }
  return messages;
}

function withGoldenHeader(content: string): string {
  return `${copyrightHeader}\n${content.trimEnd()}\n`;
}

function blessTree(bridge: DeclgenBridge, actualDir: string, goldenDir: string): string[] {
  const written: string[] = [];
  fs.rmSync(goldenDir, { recursive: true, force: true });
  for (const abs of walkFiles(actualDir)) {
    const rel = path.relative(actualDir, abs);
    const target = path.join(goldenDir, rel);
    fs.mkdirSync(path.dirname(target), { recursive: true });
    fs.writeFileSync(target, withGoldenHeader(bridge.stripComments(fs.readFileSync(abs, 'utf-8'), abs)));
    written.push(rel);
  }
  return written;
}

function runFixture(
  bridge: DeclgenBridge,
  testCase: CaseNode,
  workDir: string,
  update: boolean,
  messages: string[],
): void {
  const outDir = path.join(workDir, 'fixture-out');
  fs.mkdirSync(outDir, { recursive: true });
  bridge.runDyn2Sta(testCase.inputFiles, testCase.inputDir, outDir);
  if (update) {
    const written = blessTree(bridge, outDir, testCase.outputDir);
    messages.push(`blessed fixture output (${written.length} file(s): ${written.join(', ') || '<none>'})`);
    return;
  }
  messages.push(...compareTrees(bridge, testCase.outputDir, outDir, 'fixture'));
}

function runSample(
  bridge: DeclgenBridge,
  testCase: CaseNode,
  workDir: string,
  update: boolean,
  messages: string[],
): void {
  if (!testCase.sampleInput) {
    return;
  }
  const outDir = path.join(workDir, 'sample-out');
  fs.mkdirSync(outDir, { recursive: true });
  bridge.runDyn2Sta([testCase.sampleInput], undefined, outDir);
  const emitted = walkFiles(outDir).filter((f) => f.endsWith('.d.ets'));
  if (emitted.length !== 1) {
    messages.push(`sample: expected exactly 1 emitted .d.ets file, got ${emitted.length}`);
    return;
  }
  const actual = emitted[0]!;
  if (update) {
    const target = testCase.sampleOutput ?? path.join(testCase.absDir, 'sample.output.d.ets');
    fs.writeFileSync(target, withGoldenHeader(bridge.stripComments(fs.readFileSync(actual, 'utf-8'), actual)));
    messages.push('blessed sample output (sample.output.d.ets)');
    return;
  }
  if (!testCase.sampleOutput || !fs.existsSync(testCase.sampleOutput)) {
    messages.push('sample: sample.output.d.ets missing (run with --update to bless)');
    return;
  }
  const expected = bridge.stripComments(fs.readFileSync(testCase.sampleOutput, 'utf-8'), testCase.sampleOutput);
  const actualContent = bridge.stripComments(fs.readFileSync(actual, 'utf-8'), actual);
  if (expected !== actualContent) {
    messages.push(`sample: content mismatch\n  ${firstDifference(expected, actualContent)}`);
  }
}

export function runCase(model: SpecModel, testCase: CaseNode, options: TestRunOptions = {}): CaseResult {
  const update = options.update ?? false;
  const messages: string[] = [];
  const direction = testCase.relPath.split('/')[0]!;
  if (testCase.pending) {
    return { relPath: testCase.relPath, status: 'skipped', messages: ['pending (documented but not tested yet)'] };
  }
  if (direction !== 'dyn2sta') {
    return {
      relPath: testCase.relPath,
      status: 'fail',
      messages: [`direction "${direction}" is not wired to a generator yet; mark the case pending`],
    };
  }
  if (testCase.inputFiles.length === 0 && !testCase.sampleInput) {
    return { relPath: testCase.relPath, status: 'fail', messages: ['case has neither input/ files nor a sample'] };
  }
  const bridge = loadBridge(model.specDir);
  const workDir = fs.mkdtempSync(path.join(os.tmpdir(), `spec-kit-${testCase.slug}-`));
  try {
    if (testCase.inputFiles.length > 0) {
      runFixture(bridge, testCase, workDir, update, messages);
    }
    runSample(bridge, testCase, workDir, update, messages);
  } catch (error) {
    messages.push(`generator error: ${error instanceof Error ? error.message : String(error)}`);
  } finally {
    fs.rmSync(workDir, { recursive: true, force: true });
  }
  const failures = messages.filter((m) => !m.startsWith('blessed'));
  const blessed = messages.some((m) => m.startsWith('blessed'));
  return {
    relPath: testCase.relPath,
    status: failures.length > 0 ? 'fail' : blessed ? 'blessed' : 'pass',
    messages,
  };
}

export function runSpecTests(model: SpecModel, options: TestRunOptions = {}): CaseResult[] {
  return model.cases
    .filter(
      (testCase) => options.filter === undefined || options.filter === '' || testCase.relPath.includes(options.filter),
    )
    .map((testCase) => runCase(model, testCase, options));
}
