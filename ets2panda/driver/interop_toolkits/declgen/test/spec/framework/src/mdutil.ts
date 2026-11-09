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
 * Small markdown helpers shared by the loader, lint and renderers. Only a
 * minimal front-matter subset (`key: value` lines between --- markers) and
 * fenced-code-block inspection are supported; spec suite documents stay
 * deliberately simple so that lint can enforce structural alignment.
 */

import * as fs from 'fs';

export interface ParsedMd {
  /** Front-matter map; empty when the file has no front matter. */
  fm: Record<string, string>;
  /** Document body after the closing --- line. */
  body: string;
}

export function parseMd(content: string): ParsedMd {
  const lines = content.split('\n');
  if (lines.length < 3 || lines[0]!.trim() !== '---') {
    return { fm: {}, body: content };
  }
  const fm: Record<string, string> = {};
  let end = -1;
  for (let i = 1; i < lines.length; i++) {
    if (lines[i]!.trim() === '---') {
      end = i;
      break;
    }
    const match = /^(\w[\w-]*)\s*:\s*(.*)$/.exec(lines[i]!.trimEnd());
    if (match) {
      fm[match[1]!] = match[2]!.trim();
    }
  }
  if (end === -1) {
    return { fm: {}, body: content };
  }
  return {
    fm,
    body: lines
      .slice(end + 1)
      .join('\n')
      .replace(/^\n+/, ''),
  };
}

export function readMd(absPath: string): ParsedMd {
  return parseMd(fs.readFileSync(absPath, 'utf-8'));
}

export interface Fence {
  /** Info string of the opening fence (e.g. "typescript"), empty for plain fences. */
  info: string;
  /** Line index of the opening fence. */
  startLine: number;
}

export function fencesOf(body: string): Fence[] {
  const result: Fence[] = [];
  const lines = body.split('\n');
  let open = -1;
  let openInfo = '';
  for (let i = 0; i < lines.length; i++) {
    const match = /^```(.*)$/.exec(lines[i]!);
    if (!match) {
      continue;
    }
    if (open === -1) {
      open = i;
      openInfo = match[1]!.trim();
    } else {
      result.push({ info: openInfo, startLine: open });
      open = -1;
      openInfo = '';
    }
  }
  if (open !== -1) {
    // Unbalanced fence: report as an info-less fence so lint can flag it.
    result.push({ info: openInfo, startLine: open });
  }
  return result;
}

const LICENSE_BLOCK = /^[ \t]*\/\*[\s\S]*?Copyright[\s\S]*?\*\/[ \t]*\r?\n?/;

/** Strip the leading OpenHarmony license block for doc injection. */
export function stripLicense(content: string): string {
  return content.replace(LICENSE_BLOCK, '');
}

/** Trim trailing whitespace per line and collapse >2 consecutive blank lines. */
export function normalizeBody(body: string): string {
  return body
    .split('\n')
    .map((line) => line.trimEnd())
    .join('\n')
    .replace(/\n{3,}/g, '\n\n')
    .trim();
}
