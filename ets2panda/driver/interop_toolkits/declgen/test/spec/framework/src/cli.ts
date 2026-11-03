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
 * spec-kit command line entry point.
 *
 *   spec-kit lint    [--spec-dir <dir>]
 *   spec-kit render  [--spec-dir <dir>] [--lang zh|en] [--structure <s>] [--check] [--all]
 *   spec-kit test    [--spec-dir <dir>] [--update] [--filter <substr>]
 *   spec-kit new     <parent/path> <slug> [--spec-dir <dir>]
 *
 * Argument parsing is deliberately hand-rolled to keep spec-kit free of
 * runtime dependencies.
 */

import * as fs from 'fs';
import * as path from 'path';

import { LoadError, loadSpec } from './loader';
import { lintSpec } from './lint';
import { LANGS, type Lang, type Structure } from './model';
import { outputDocName, renderDoc } from './render';
import { runSpecTests } from './testcmd';
import { scaffoldCase } from './newcmd';

interface ParsedArgs {
  positional: string[];
  flags: Map<string, string | boolean>;
}

function consumeFlag(arg: string, argv: string[], index: number, flags: Map<string, string | boolean>): number {
  const eq = arg.indexOf('=');
  if (eq !== -1) {
    flags.set(arg.slice(2, eq), arg.slice(eq + 1));
    return index;
  }
  const next: string | undefined = argv[index + 1];
  if (next !== undefined && !next.startsWith('--')) {
    flags.set(arg.slice(2), next);
    return index + 1;
  }
  flags.set(arg.slice(2), true);
  return index;
}

function parseArgs(argv: string[]): ParsedArgs {
  const positional: string[] = [];
  const flags = new Map<string, string | boolean>();
  for (let i = 0; i < argv.length; i++) {
    const arg = argv[i]!;
    if (!arg.startsWith('--')) {
      positional.push(arg);
      continue;
    }
    i = consumeFlag(arg, argv, i, flags);
  }
  return { positional, flags };
}

function specDirOf(flags: Map<string, string | boolean>): string {
  const value = flags.get('spec-dir');
  return typeof value === 'string' ? path.resolve(value) : path.resolve(process.cwd(), 'test', 'spec');
}

function reportLoadError(error: LoadError): void {
  console.error('spec suite failed to load:');
  for (const issue of error.issues) {
    console.error(`  [load] ${issue.relPath}: ${issue.message}`);
  }
}

function cmdLint(specDir: string): number {
  let model;
  try {
    model = loadSpec(specDir);
  } catch (error) {
    if (error instanceof LoadError) {
      reportLoadError(error);
      return 1;
    }
    throw error;
  }
  const issues = lintSpec(model);
  for (const issue of issues) {
    console.log(`  [${issue.severity}] ${issue.relPath}: ${issue.message}`);
  }
  const errors = issues.filter((issue) => issue.severity === 'error');
  console.log(`lint: ${errors.length} error(s), ${issues.length - errors.length} warning(s)`);
  return errors.length > 0 ? 1 : 0;
}

function cmdRender(specDir: string, flags: Map<string, string | boolean>): number {
  let model;
  try {
    model = loadSpec(specDir);
  } catch (error) {
    if (error instanceof LoadError) {
      reportLoadError(error);
      return 1;
    }
    throw error;
  }
  const issues = lintSpec(model).filter((issue) => issue.severity === 'error');
  if (issues.length > 0) {
    console.error(`render: ${issues.length} lint error(s) must be fixed first`);
    return 1;
  }
  const check = flags.get('check') === true;
  const all = flags.get('all') === true;
  const langFlag = flags.get('lang');
  const structureFlag = flags.get('structure') as Structure | undefined;
  const langs: Lang[] = all || !langFlag ? LANGS : [langFlag as Lang];
  const docsDir = path.join(specDir, 'docs');
  fs.mkdirSync(docsDir, { recursive: true });
  let failed = false;
  for (const lang of langs) {
    const content = renderDoc(model, lang, 'md', structureFlag);
    const target = path.join(docsDir, outputDocName(model.config.docId, lang, 'md'));
    if (check) {
      if (!fs.existsSync(target)) {
        console.error(`render --check: ${target} does not exist`);
        failed = true;
        continue;
      }
      const existing = fs.readFileSync(target, 'utf-8');
      if (existing !== content) {
        console.error(`render --check: ${path.basename(target)} is stale; re-run spec:render`);
        failed = true;
        continue;
      }
      console.log(`render --check: ${path.basename(target)} up to date`);
    } else {
      fs.writeFileSync(target, content);
      console.log(`rendered ${path.basename(target)} (${content.length} bytes)`);
    }
  }
  return failed ? 1 : 0;
}

function cmdTest(specDir: string, flags: Map<string, string | boolean>): number {
  let model;
  try {
    model = loadSpec(specDir);
  } catch (error) {
    if (error instanceof LoadError) {
      reportLoadError(error);
      return 1;
    }
    throw error;
  }
  const update = flags.get('update') === true;
  const issues = lintSpec(model, { blessing: update }).filter((issue) => issue.severity === 'error');
  if (issues.length > 0) {
    console.error(`test: ${issues.length} lint error(s) must be fixed first`);
    return 1;
  }
  const filter = typeof flags.get('filter') === 'string' ? (flags.get('filter') as string) : undefined;
  const results = runSpecTests(model, { update, filter });
  let failed = 0;
  for (const result of results) {
    const tag =
      result.status === 'pass'
        ? 'PASS'
        : result.status === 'blessed'
          ? 'BLESS'
          : result.status === 'skipped'
            ? 'SKIP '
            : 'FAIL';
    console.log(`[${tag}] ${result.relPath}`);
    for (const message of result.messages) {
      console.log(`       ${message}`);
    }
    if (result.status === 'fail') {
      failed++;
    }
  }
  console.log(`test: ${results.length - failed} ok, ${failed} failed`);
  return failed > 0 ? 1 : 0;
}

function cmdNew(specDir: string, positional: string[]): number {
  const [parentPath, slug] = positional;
  if (!parentPath || !slug) {
    console.error('usage: spec-kit new <parent/path> <slug>');
    return 1;
  }
  const relPath = scaffoldCase(specDir, parentPath, slug);
  console.log(`created ${relPath} (remember to fill in titles/descriptions, then run test --update to bless outputs)`);
  return 0;
}

export function main(argv: string[]): number {
  const [command, ...rest] = argv;
  const { positional, flags } = parseArgs(rest);
  const specDir = specDirOf(flags);
  switch (command) {
    case 'lint':
      return cmdLint(specDir);
    case 'render':
      return cmdRender(specDir, flags);
    case 'test':
      return cmdTest(specDir, flags);
    case 'new':
      return cmdNew(specDir, positional);
    default:
      console.error('usage: spec-kit <lint|render|test|new> [options]');
      return command === undefined || command === 'help' || command === '--help' ? 0 : 1;
  }
}

if (require.main === module) {
  process.exit(main(process.argv.slice(2)));
}
