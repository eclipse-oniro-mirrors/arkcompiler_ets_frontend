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
 * Content-level lint rules over the canonical model. Load-time structural
 * errors (SUMMARY consistency, direction placement) are enforced by the
 * loader; everything checkable-but-loadable lives here.
 */

import * as fs from 'fs';

import {
  compatRank,
  isCase,
  isSection,
  LANGS,
  type CaseNode,
  type Lang,
  type SectionNode,
  type SpecModel,
  type SpecNode,
} from './model';
import { fencesOf } from './mdutil';

export interface LintIssue {
  severity: 'error' | 'warning';
  relPath: string;
  message: string;
}

export interface LintOptions {
  /**
   * Bless mode (--update): missing golden files are downgraded to warnings
   * because the upcoming bless run creates them. Keeping the placeholder
   * check active still catches typos in the sample file names.
   */
  blessing?: boolean;
}

const PLACEHOLDER = /\{\{\s*(input|output|sample:input|sample:output)\s*\}\}/g;

function pushIssue(issues: LintIssue[], relPath: string, message: string, severity: 'error' | 'warning' = 'error'): void {
  issues.push({ severity, relPath, message });
}

function checkTitles(titles: Record<Lang, string>, relPath: string, baseName: string, issues: LintIssue[]): void {
  for (const lang of LANGS) {
    if (titles[lang].trim() === '') {
      pushIssue(issues, relPath, `${baseName}.${lang}.md front-matter title is empty`);
    }
  }
}

function checkFenceAlignment(
  zhBody: string,
  enBody: string,
  relPath: string,
  context: string,
  issues: LintIssue[],
): void {
  const zhFences = fencesOf(zhBody);
  const enFences = fencesOf(enBody);
  if (zhFences.length !== enFences.length) {
    pushIssue(issues, relPath, `fence count mismatch${context}: zh has ${zhFences.length}, en has ${enFences.length}`);
    return;
  }
  for (let i = 0; i < zhFences.length; i++) {
    if (zhFences[i]!.info !== enFences[i]!.info) {
      pushIssue(
        issues,
        relPath,
        `fence #${i + 1} info mismatch${context}: zh "${zhFences[i]!.info}" vs en "${enFences[i]!.info}"`,
      );
    }
  }
}

function checkAnchorRefs(model: SpecModel, body: string, relPath: string, issues: LintIssue[]): void {
  for (const match of body.matchAll(/\{#([a-z0-9-]+)\}/g)) {
    if (!model.anchors.has(match[1]!)) {
      pushIssue(issues, relPath, `unknown anchor reference: {#${match[1]}}`);
    }
  }
}

function collectPlaceholders(body: string): Set<string> {
  const seen = new Set<string>();
  for (const match of body.matchAll(PLACEHOLDER)) {
    seen.add(match[1]!);
  }
  return seen;
}

function checkFixturePlaceholders(node: CaseNode, seen: Set<string>, issues: LintIssue[]): void {
  if (!seen.has('input') && !seen.has('output')) {
    return;
  }
  if (node.inputFiles.length === 0) {
    pushIssue(issues, node.relPath, `{{input}}/{{output}} used but input/ is empty`);
    return;
  }
  if (node.inputFiles.length > 1 && !node.sampleInput) {
    pushIssue(
      issues,
      node.relPath,
      `multi-file fixture referenced via {{input}}; add a sample pair and use {{sample:input}}/{{sample:output}}`,
    );
  }
}

function checkSamplePlaceholders(node: CaseNode, seen: Set<string>, issues: LintIssue[], options: LintOptions): void {
  if (seen.has('sample:input') && !node.sampleInput) {
    pushIssue(issues, node.relPath, `{{sample:input}} used but sample.input.ets is missing`);
  }
  if (seen.has('sample:output') && !node.sampleOutput) {
    pushIssue(
      issues,
      node.relPath,
      `{{sample:output}} used but sample.output.d.ets is missing (bless will create it)`,
      options.blessing ? 'warning' : 'error',
    );
  }
}

function checkCasePlaceholders(model: SpecModel, node: CaseNode, issues: LintIssue[], options: LintOptions): void {
  for (const lang of LANGS) {
    const body = node.desc[lang].body;
    const seen = collectPlaceholders(body);
    checkFixturePlaceholders(node, seen, issues);
    checkSamplePlaceholders(node, seen, issues, options);
    checkAnchorRefs(model, body, node.relPath, issues);
  }
}

function countLines(absPath: string): number {
  return fs.readFileSync(absPath, 'utf-8').split('\n').length;
}

function checkSampleThresholds(node: CaseNode, issues: LintIssue[]): void {
  const sampleLines = node.sampleInput ? countLines(node.sampleInput) : 0;
  if (sampleLines > 40) {
    pushIssue(issues, node.relPath, `sample.input.ets has ${sampleLines} lines (> 40); keep doc examples small`, 'warning');
  }
  if (node.inputFiles.length > 1 && !node.sampleInput) {
    pushIssue(issues, node.relPath, 'multi-file fixture without sample pair', 'warning');
  }
}

function checkCaseNode(model: SpecModel, node: CaseNode, issues: LintIssue[], options: LintOptions): void {
  checkTitles(
    { zh: node.desc.zh.title, en: node.desc.en.title },
    node.relPath,
    'desc',
    issues,
  );
  checkFenceAlignment(node.desc.zh.body, node.desc.en.body, node.relPath, '', issues);
  checkCasePlaceholders(model, node, issues, options);
  checkSampleThresholds(node, issues);
}

function checkSectionNode(model: SpecModel, node: SectionNode, issues: LintIssue[], options: LintOptions): void {
  checkTitles(node.titles, node.relPath, 'section', issues);
  checkFenceAlignment(node.narrative.zh, node.narrative.en, node.relPath, ' in narrative', issues);
  for (const lang of LANGS) {
    checkAnchorRefs(model, node.narrative[lang], node.relPath, issues);
  }
  checkCompatOrder(node, issues);
  node.children.forEach((child) => checkNode(model, child, issues, options));
}

function checkNode(model: SpecModel, node: SpecNode, issues: LintIssue[], options: LintOptions): void {
  if (isCase(node)) {
    checkCaseNode(model, node, issues, options);
    return;
  }
  checkSectionNode(model, node, issues, options);
}

function checkCompatOrder(section: SectionNode, issues: LintIssue[]): void {
  let lastRank = -1;
  for (const child of section.children) {
    if (!isCase(child)) {
      continue;
    }
    const rank = compatRank(child.compat);
    if (rank < lastRank) {
      pushIssue(
        issues,
        child.relPath,
        `compat order violated: "${child.compat}" appears after a later scenario (expected 完全兼容 → 需要转换 → 完全不兼容)`,
      );
    }
    lastRank = Math.max(lastRank, rank);
  }
}

function topicMapOf(direction: SectionNode): Map<string, SectionNode> {
  const map = new Map<string, SectionNode>();
  for (const child of direction.children) {
    if (isSection(child)) {
      map.set(child.slug, child);
    }
  }
  return map;
}

function checkTopicTitles(node: SectionNode, counterpart: SectionNode, issues: LintIssue[]): void {
  for (const lang of LANGS) {
    if (node.titles[lang] !== counterpart.titles[lang]) {
      pushIssue(
        issues,
        counterpart.relPath,
        `topic title mismatch with ${node.relPath} (${lang}): "${counterpart.titles[lang]}" vs "${node.titles[lang]}"`,
      );
    }
  }
}

/** Titles of the same topic must match across direction groups. */
function checkCrossDirectionTitles(model: SpecModel, issues: LintIssue[]): void {
  const directions = model.roots.filter(isSection).filter((node) => node.direction);
  if (directions.length < 2) {
    return;
  }
  const topicMaps = directions.map(topicMapOf);
  const [first, ...rest] = topicMaps;
  for (const [slug, node] of first!) {
    for (const other of rest) {
      const counterpart = other.get(slug);
      if (counterpart) {
        checkTopicTitles(node, counterpart, issues);
      }
    }
  }
}

export function lintSpec(model: SpecModel, options: LintOptions = {}): LintIssue[] {
  const issues: LintIssue[] = [];
  model.roots.forEach((root) => checkNode(model, root, issues, options));
  checkCrossDirectionTitles(model, issues);
  return issues;
}
