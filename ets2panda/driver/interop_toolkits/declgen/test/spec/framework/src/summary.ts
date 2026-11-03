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
 * SUMMARY.md is the single ordering truth of the spec suite (mdBook/GitBook
 * precedent). It lists every node directory exactly once, in document order;
 * indentation encodes tree depth. This module parses and serializes it.
 */

import * as fs from 'fs';
import * as path from 'path';

export interface SummaryEntry {
  /** Relative posix path from sections/ (e.g. "dyn2sta/ambient/add-declare"). */
  relPath: string;
  depth: number;
}

export interface SummaryDoc {
  /** Header lines (title, comments) kept verbatim on serialization. */
  header: string;
  entries: SummaryEntry[];
}

interface SummaryIssue {
  line: number;
  message: string;
}

const BULLET_RE = /^(\s*)-\s+(\S+)\s*$/;

function collectHeaderLine(raw: string, headerLines: string[]): void {
  // Before the first bullet every line is header content; blank lines are
  // kept as empty strings so the header round-trips through serialization.
  headerLines.push(raw.trimEnd());
}

function reportUnexpectedContent(raw: string, lineIdx: number, issues: SummaryIssue[]): void {
  if (raw.trim() !== '') {
    issues.push({ line: lineIdx + 1, message: `unexpected content after summary list: "${raw.trimEnd()}"` });
  }
}

function collectEntry(
  bullet: RegExpExecArray,
  lineIdx: number,
  stack: string[],
  entries: SummaryEntry[],
  issues: SummaryIssue[],
): void {
  const spaces = bullet[1]!.length;
  if (spaces % 2 !== 0) {
    issues.push({ line: lineIdx + 1, message: `indentation must be multiples of 2 spaces` });
    return;
  }
  const depth = spaces / 2;
  if (depth > stack.length) {
    issues.push({ line: lineIdx + 1, message: `indentation skips a level` });
    return;
  }
  stack.length = depth;
  stack.push(bullet[2]!);
  entries.push({ relPath: stack.join('/'), depth });
}

export function parseSummary(content: string): { doc: SummaryDoc; issues: SummaryIssue[] } {
  const lines = content.split('\n');
  const headerLines: string[] = [];
  const entries: SummaryEntry[] = [];
  const issues: SummaryIssue[] = [];
  const stack: string[] = [];
  let seenFirstBullet = false;

  for (let i = 0; i < lines.length; i++) {
    const bullet = BULLET_RE.exec(lines[i]!);
    if (!bullet) {
      if (seenFirstBullet) {
        reportUnexpectedContent(lines[i]!, i, issues);
      } else {
        collectHeaderLine(lines[i]!, headerLines);
      }
      continue;
    }
    seenFirstBullet = true;
    collectEntry(bullet, i, stack, entries, issues);
  }

  return { doc: { header: headerLines.join('\n').trimEnd(), entries }, issues };
}

export function serializeSummary(doc: SummaryDoc): string {
  const lines: string[] = [];
  if (doc.header !== '') {
    lines.push(...doc.header.split('\n'), '');
  }
  for (const entry of doc.entries) {
    lines.push(`${'  '.repeat(entry.depth)}- ${entry.relPath.split('/').pop()}`);
  }
  return lines.join('\n').replace(/\n{3,}/g, '\n\n') + '\n';
}

export function loadSummary(sectionsDir: string): { doc: SummaryDoc; issues: SummaryIssue[] } {
  const summaryPath = path.resolve(sectionsDir, '..', 'SUMMARY.md');
  if (!fs.existsSync(summaryPath)) {
    return {
      doc: { header: '# Spec summary', entries: [] },
      issues: [{ line: 0, message: `SUMMARY.md not found: ${summaryPath}` }],
    };
  }
  return parseSummary(fs.readFileSync(summaryPath, 'utf-8'));
}

export function summaryPathOf(sectionsDir: string): string {
  return path.resolve(sectionsDir, '..', 'SUMMARY.md');
}
