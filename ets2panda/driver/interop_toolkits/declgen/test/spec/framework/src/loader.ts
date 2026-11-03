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
 * Loads test/spec into the canonical model:
 *
 *   1. Scan sections/ from disk into an unordered node map (node kind comes
 *      from case.json presence; direction groups carry node.json).
 *   2. Parse SUMMARY.md and validate it against the disk tree (bidirectional
 *      consistency, enforced here as hard load errors).
 *   3. Build the ordered model, attaching per-language titles/narratives.
 *
 * Structural violations are reported as load errors; content-level checks
 * (fence alignment, placeholder validity, ...) live in lint.ts.
 */

import * as fs from 'fs';
import * as path from 'path';

import {
  type CaseNode,
  COMPAT_VALUES,
  type DocText,
  LANGS,
  type Lang,
  type SectionNode,
  type SpecConfig,
  type SpecModel,
  type SpecNode,
  type Structure,
} from './model';
import { readMd } from './mdutil';
import { loadSummary, type SummaryDoc } from './summary';

export interface LoadIssue {
  relPath: string;
  message: string;
}

export class LoadError extends Error {
  issues: LoadIssue[];
  constructor(issues: LoadIssue[]) {
    const details = issues
      .slice(0, 5)
      .map((issue) => `${issue.relPath}: ${issue.message}`)
      .join('; ');
    super(`failed to load spec suite (${issues.length} issue(s)): ${details}`);
    this.issues = issues;
  }
}

const DIRECTION_KIND = 'direction';

function listDirs(absDir: string): string[] {
  return fs
    .readdirSync(absDir, { withFileTypes: true })
    .filter((entry) => entry.isDirectory())
    .map((entry) => entry.name)
    .sort();
}

function readNodeJson(absDir: string): Record<string, unknown> | undefined {
  const nodeJsonPath = path.join(absDir, 'node.json');
  if (!fs.existsSync(nodeJsonPath)) {
    return undefined;
  }
  try {
    return JSON.parse(fs.readFileSync(nodeJsonPath, 'utf-8')) as Record<string, unknown>;
  } catch {
    return undefined;
  }
}

function readDocPair(absDir: string, baseName: string): Record<Lang, DocText> | string {
  const result = {} as Record<Lang, DocText>;
  for (const lang of LANGS) {
    const absPath = path.join(absDir, `${baseName}.${lang}.md`);
    if (!fs.existsSync(absPath)) {
      return `missing ${baseName}.${lang}.md`;
    }
    const parsed = readMd(absPath);
    result[lang] = { absPath, title: parsed.fm['title'] ?? '', body: parsed.body };
  }
  return result;
}

function listFilesRecursive(absDir: string): string[] {
  if (!fs.existsSync(absDir)) {
    return [];
  }
  const result: string[] = [];
  const walk = (dir: string): void => {
    for (const entry of fs.readdirSync(dir, { withFileTypes: true })) {
      const full = path.join(dir, entry.name);
      if (entry.isDirectory()) {
        walk(full);
      } else {
        result.push(full);
      }
    }
  };
  walk(absDir);
  return result.sort();
}

interface RawNode {
  relPath: string;
  absDir: string;
  slug: string;
  isCase: boolean;
  direction: boolean;
}

function loadConfig(specDir: string): SpecConfig {
  const configPath = path.join(specDir, 'spec.config.json');
  const raw = JSON.parse(fs.readFileSync(configPath, 'utf-8')) as Record<string, never>;
  const title = (raw['title'] ?? {}) as Record<string, string>;
  const structure = (raw['structure'] ?? 'topic-major') as Structure;
  if (structure !== 'topic-major' && structure !== 'direction-major') {
    throw new LoadError([{ relPath: 'spec.config.json', message: `unknown structure: ${structure}` }]);
  }
  return {
    docId: raw['docId'] ?? 'spec',
    title: { zh: title['zh'] ?? '', en: title['en'] ?? '' },
    frontMatter: (raw['frontMatter'] ?? []) as string[],
    structure,
  };
}

function scanNode(rawMap: Map<string, RawNode>, issues: LoadIssue[], absDir: string, relPath: string): void {
  for (const slug of listDirs(absDir)) {
    const childAbs = path.join(absDir, slug);
    const childRel = relPath === '' ? slug : `${relPath}/${slug}`;
    const nodeJson = readNodeJson(childAbs);
    const isCase = fs.existsSync(path.join(childAbs, 'case.json'));
    const direction = nodeJson?.['kind'] === DIRECTION_KIND;
    if (nodeJson && nodeJson['kind'] !== DIRECTION_KIND) {
      issues.push({ relPath: childRel, message: `node.json has unknown kind: ${String(nodeJson['kind'])}` });
    }
    rawMap.set(childRel, { relPath: childRel, absDir: childAbs, slug, isCase, direction });
    // Case directories hold fixture content (input/, output/); only
    // section directories can contain child nodes.
    if (!isCase) {
      scanNode(rawMap, issues, childAbs, childRel);
    }
  }
}

/** Phase 1: scan sections/ from disk into an unordered node map. */
function scanDisk(sectionsDir: string, issues: LoadIssue[]): Map<string, RawNode> {
  const rawMap = new Map<string, RawNode>();
  scanNode(rawMap, issues, sectionsDir, '');
  return rawMap;
}

/** Phase 2a: SUMMARY entries must match the disk tree bidirectionally. */
function checkSummaryEntries(rawMap: Map<string, RawNode>, summary: SummaryDoc, issues: LoadIssue[]): void {
  const summarySet = new Set<string>();
  for (const entry of summary.entries) {
    if (summarySet.has(entry.relPath)) {
      issues.push({ relPath: 'SUMMARY.md', message: `duplicate entry: ${entry.relPath}` });
    }
    summarySet.add(entry.relPath);
    if (!rawMap.has(entry.relPath)) {
      issues.push({ relPath: 'SUMMARY.md', message: `entry has no directory: ${entry.relPath}` });
    }
  }
  for (const relPath of rawMap.keys()) {
    if (!summarySet.has(relPath)) {
      issues.push({ relPath: 'SUMMARY.md', message: `directory missing from SUMMARY.md: ${relPath}` });
    }
  }
}

function checkSummaryDoc(sectionsDir: string, rawMap: Map<string, RawNode>, issues: LoadIssue[]): SummaryDoc {
  const { doc: summary, issues: summaryIssues } = loadSummary(sectionsDir);
  for (const issue of summaryIssues) {
    issues.push({ relPath: 'SUMMARY.md', message: `line ${issue.line}: ${issue.message}` });
  }
  checkSummaryEntries(rawMap, summary, issues);
  return summary;
}

/** Phase 2b: structural rules the ordered model depends on. */
function checkStructure(rawMap: Map<string, RawNode>, issues: LoadIssue[]): void {
  for (const raw of rawMap.values()) {
    const depth = raw.relPath.split('/').length;
    if (raw.direction && depth !== 1) {
      issues.push({ relPath: raw.relPath, message: 'direction group must sit directly under sections/' });
    }
    if (raw.direction && raw.isCase) {
      issues.push({ relPath: raw.relPath, message: 'direction group cannot be a case (case.json present)' });
    }
    const parentRel = raw.relPath.split('/').slice(0, -1).join('/');
    if (parentRel !== '' && rawMap.get(parentRel)?.direction && raw.isCase) {
      issues.push({ relPath: raw.relPath, message: 'direction group children must be topic sections, not cases' });
    }
  }
}

interface BuildContext {
  rawMap: Map<string, RawNode>;
  summary: SummaryDoc;
  anchors: Set<string>;
}

/** Phase 3a: build one case node (fixtures, docs, compat). */
function buildCaseNode(raw: RawNode, relPath: string, anchor: string): CaseNode {
  const caseJson = JSON.parse(fs.readFileSync(path.join(raw.absDir, 'case.json'), 'utf-8')) as Record<string, never>;
  const compat = (caseJson['compat'] ?? 'requires-transform') as CaseNode['compat'];
  if (!COMPAT_VALUES.includes(compat)) {
    throw new LoadError([{ relPath, message: `case.json has invalid compat: ${String(compat)}` }]);
  }
  const desc = readDocPair(raw.absDir, 'desc');
  if (typeof desc === 'string') {
    throw new LoadError([{ relPath, message: desc }]);
  }
  const inputDir = path.join(raw.absDir, 'input');
  const sampleInput = path.join(raw.absDir, 'sample.input.ets');
  const sampleOutput = path.join(raw.absDir, 'sample.output.d.ets');
  return {
    kind: 'case',
    slug: raw.slug,
    relPath,
    absDir: raw.absDir,
    anchor,
    compat,
    tags: (caseJson['tags'] ?? []) as string[],
    pending: Boolean(caseJson['pending']),
    inputDir,
    outputDir: path.join(raw.absDir, 'output'),
    inputFiles: listFilesRecursive(inputDir),
    sampleInput: fs.existsSync(sampleInput) ? sampleInput : undefined,
    sampleOutput: fs.existsSync(sampleOutput) ? sampleOutput : undefined,
    desc,
  };
}

/** Phase 3b: build one section node, recursing into SUMMARY-ordered children. */
function buildSectionNode(ctx: BuildContext, raw: RawNode, relPath: string, anchor: string): SectionNode {
  const docs = readDocPair(raw.absDir, 'section');
  if (typeof docs === 'string') {
    throw new LoadError([{ relPath, message: docs }]);
  }
  const children: SpecNode[] = [];
  const node: SectionNode = {
    kind: 'section',
    slug: raw.slug,
    relPath,
    absDir: raw.absDir,
    anchor,
    direction: raw.direction,
    titles: { zh: docs.zh.title, en: docs.en.title },
    narrative: { zh: docs.zh.body, en: docs.en.body },
    children,
  };
  const prefix = `${relPath}/`;
  const depth = relPath.split('/').length;
  for (const entry of ctx.summary.entries) {
    if (entry.depth === depth && entry.relPath.startsWith(prefix)) {
      children.push(buildNode(ctx, entry.relPath));
    }
  }
  return node;
}

function buildNode(ctx: BuildContext, relPath: string): SpecNode {
  const raw = ctx.rawMap.get(relPath)!;
  const anchor = relPath.split('/').join('-');
  ctx.anchors.add(anchor);
  if (raw.isCase) {
    return buildCaseNode(raw, relPath, anchor);
  }
  return buildSectionNode(ctx, raw, relPath, anchor);
}

function collectCases(node: SpecNode, cases: CaseNode[]): void {
  if (node.kind === 'case') {
    cases.push(node);
    return;
  }
  node.children.forEach((child) => collectCases(child, cases));
}

/** Phase 3: build the ordered model from SUMMARY. */
function buildModel(
  specDir: string,
  sectionsDir: string,
  config: SpecConfig,
  rawMap: Map<string, RawNode>,
  summary: SummaryDoc,
): SpecModel {
  const ctx: BuildContext = { rawMap, summary, anchors: new Set<string>() };
  const roots: SpecNode[] = [];
  for (const entry of summary.entries) {
    if (entry.depth === 0) {
      roots.push(buildNode(ctx, entry.relPath));
    }
  }
  const cases: CaseNode[] = [];
  roots.forEach((node) => collectCases(node, cases));
  return { specDir, sectionsDir, config, roots, cases, anchors: ctx.anchors };
}

export function loadSpec(specDir: string): SpecModel {
  const sectionsDir = path.join(specDir, 'sections');
  if (!fs.existsSync(sectionsDir)) {
    throw new LoadError([{ relPath: 'sections', message: 'sections directory not found' }]);
  }
  const issues: LoadIssue[] = [];
  const config = loadConfig(specDir);
  const rawMap = scanDisk(sectionsDir, issues);
  const summary = checkSummaryDoc(sectionsDir, rawMap, issues);
  checkStructure(rawMap, issues);
  if (issues.length > 0) {
    throw new LoadError(issues);
  }
  return buildModel(specDir, sectionsDir, config, rawMap, summary);
}
