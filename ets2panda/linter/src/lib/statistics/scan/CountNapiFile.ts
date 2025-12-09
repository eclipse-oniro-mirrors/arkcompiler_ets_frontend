/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

import * as fs from 'fs';
import * as path from 'path';
import { runWithIOLimit, mapWithLimit } from './IoLimiter';
import { Logger } from '../../Logger';
import type { NapiFileStatisticInfo } from './NapiFileStatisticInfo';

const EXTENSIONS = ['.c', '.cpp', '.cc', '.cxx', '.h', '.hpp', '.hh', '.hxx'];

const SINGLE_LINE_COMMENT_REGEX = /\/\/.*/g;
const MULTI_LINE_COMMENT_REGEX = /\/\*[\s\S]*?\*\//g;

const DEFAULT_STATISTICS: NapiFileStatisticInfo = {
  totalFiles: 0,
  totalLines: 0,
  napiFiles: 0,
  napiLines: 0,
  napiFileLines: 0
};

function removeComments(content: string): string {
  return content.replace(MULTI_LINE_COMMENT_REGEX, '').replace(SINGLE_LINE_COMMENT_REGEX, '');
}

function countLinesFromContent(content: string): number {
  const contentWithoutComments = removeComments(content);
  const validLines = contentWithoutComments.split('\n').filter((line) => {
    return line.trim();
  });
  return validLines.length;
}

function countNapiLinesFromContent(content: string): number {
  const lines = content.split('\n');
  const napiLines = new Set<string>();

  for (const line of lines) {
    if (line.toLowerCase().includes('napi')) {
      napiLines.add(line);
    }
  }

  return napiLines.size;
}

async function analyzeDirectoryAsync(directory: string): Promise<NapiFileStatisticInfo> {
  const dirQueue: string[] = [directory];
  const allResults: NapiFileStatisticInfo[] = [];
  const MAX_CONCURRENT_FILES = 32;

  while (dirQueue.length > 0) {
    const currentDir = dirQueue.shift()!;
    const entries = await runWithIOLimit(() => {
      return fs.promises.readdir(currentDir, { withFileTypes: true });
    });

    const filesToProcess: string[] = [];
    for (const entry of entries) {
      const fullPath = path.join(currentDir, entry.name);
      if (entry.isDirectory()) {
        dirQueue.push(fullPath);
      } else if (isTargetFile(entry.name)) {
        filesToProcess.push(fullPath);
      }
    }

    const fileResults = await mapWithLimit(filesToProcess, MAX_CONCURRENT_FILES, async (filePath) => {
      return processFile(filePath);
    });
    allResults.push(...fileResults);
  }

  return allResults.reduce(
    (acc, cur) => {
      acc.totalFiles += cur.totalFiles;
      acc.totalLines += cur.totalLines;
      if (cur.napiFiles > 0) {
        acc.napiFiles += cur.napiFiles;
        acc.napiLines += cur.napiLines;
        acc.napiFileLines += cur.napiFileLines;
      }
      return acc;
    },
    { ...DEFAULT_STATISTICS }
  );
}

async function processFile(filePath: string): Promise<NapiFileStatisticInfo> {
  const result: NapiFileStatisticInfo = {
    totalFiles: 1,
    totalLines: 0,
    napiFiles: 0,
    napiLines: 0,
    napiFileLines: 0
  };

  try {
    const content = await runWithIOLimit(() => {
      return fs.promises.readFile(filePath, 'utf-8');
    });
    const [lines, napiCount] = await Promise.all([
      Promise.resolve(countLinesFromContent(content)),
      Promise.resolve(countNapiLinesFromContent(content))
    ]);
    result.totalLines = lines;
    if (napiCount > 0) {
      result.napiFiles = 1;
      result.napiLines = napiCount;
      result.napiFileLines = lines;
    }
  } catch (e) {
    Logger.error(`Error processing ${filePath}: ${e}`);
  }
  return result;
}

function isTargetFile(filename: string): boolean {
  return EXTENSIONS.some((ext) => {
    return filename.endsWith(ext);
  });
}

export async function countNapiFiles(directory: string): Promise<NapiFileStatisticInfo> {
  try {
    const stat = await runWithIOLimit(() => {
      return fs.promises.stat(directory);
    });
    if (!stat.isDirectory()) {
      Logger.error('The provided path is not a directory!');
      return DEFAULT_STATISTICS;
    }
    return await analyzeDirectoryAsync(directory);
  } catch (e) {
    Logger.error(`Error accessing directory ${directory}: ${e}`);
    return DEFAULT_STATISTICS;
  }
}
