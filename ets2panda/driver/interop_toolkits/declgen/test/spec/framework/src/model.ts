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
 * Canonical model of the spec test suite. The loader walks test/spec/sections,
 * validates it against SUMMARY.md, and produces this model; lint, render and
 * test commands all consume it.
 */

export type Lang = 'zh' | 'en';
export const LANGS: Lang[] = ['zh', 'en'];

export type Compat = 'fully-compatible' | 'requires-transform' | 'incompatible';
export const COMPAT_VALUES: Compat[] = ['fully-compatible', 'requires-transform', 'incompatible'];

export type Structure = 'topic-major' | 'direction-major';

export interface SpecConfig {
  docId: string;
  title: Record<Lang, string>;
  frontMatter: string[];
  structure: Structure;
}

export interface DocText {
  absPath: string;
  title: string;
  body: string;
}

interface NodeBase {
  /** Directory name, unique among siblings. */
  slug: string;
  /** Relative posix path from sections/ (e.g. "dyn2sta/ambient/add-declare"). */
  relPath: string;
  absDir: string;
  /** Cross-reference anchor derived from the slug path. */
  anchor: string;
}

export interface CaseNode extends NodeBase {
  kind: 'case';
  compat: Compat;
  tags: string[];
  /** Pending cases are rendered into docs but skipped by the test runner. */
  pending: boolean;
  inputDir: string;
  outputDir: string;
  inputFiles: string[];
  sampleInput?: string;
  sampleOutput?: string;
  desc: Record<Lang, DocText>;
}

export interface SectionNode extends NodeBase {
  kind: 'section';
  /** True when node.json declares {"kind": "direction"} (dyn2sta / sta2dyn groups). */
  direction: boolean;
  titles: Record<Lang, string>;
  narrative: Record<Lang, string>;
  children: SpecNode[];
}

export type SpecNode = CaseNode | SectionNode;

export interface SpecModel {
  specDir: string;
  sectionsDir: string;
  config: SpecConfig;
  /** Root nodes ordered per SUMMARY.md. */
  roots: SpecNode[];
  /** All cases in SUMMARY pre-order. */
  cases: CaseNode[];
  /** All anchors in the suite. */
  anchors: Set<string>;
}

export function isCase(node: SpecNode): node is CaseNode {
  return node.kind === 'case';
}

export function isSection(node: SpecNode): node is SectionNode {
  return node.kind === 'section';
}

/** Scenario labels rendered before a case description. */
export function compatLabel(compat: Compat, lang: Lang): string {
  if (lang === 'zh') {
    switch (compat) {
      case 'fully-compatible':
        return '**完全兼容**的场景：';
      case 'requires-transform':
        return '**需要转换**的场景：';
      case 'incompatible':
        return '**完全不兼容**的场景：';
    }
  }
  switch (compat) {
    case 'fully-compatible':
      return 'Fully compatible:';
    case 'requires-transform':
      return 'Requires transformation:';
    case 'incompatible':
      return 'Incompatible:';
  }
}

/** Canonical ordering of sibling scenarios. */
export function compatRank(compat: Compat): number {
  return COMPAT_VALUES.indexOf(compat);
}
