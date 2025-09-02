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

async function countLines(filePath: string): Promise<number> {
  try {
    const content = await fs.promises.readFile(filePath, 'utf-8');
    const contentWithoutComments = removeComments(content);
    const validLines = contentWithoutComments.split('\n').filter((line) => {
      return line.trim();
    });
    return validLines.length;
  } catch (e) {
    Logger.error(`Error reading ${filePath}: ${e}`);
    return 0;
  }
}

async function countNapiLines(filePath: string): Promise<number> {
  try {
    const content = await fs.promises.readFile(filePath, 'utf-8');
    const lines = content.split('\n');
    const napiLines = new Set<string>();

    for (const line of lines) {
      if (line.toLowerCase().includes('napi')) {
        napiLines.add(line);
      }
    }

    return napiLines.size;
  } catch (e) {
    Logger.error(`Error reading ${filePath}: ${e}`);
    return 0;
  }
}

async function analyzeDirectoryAsync(directory: string): Promise<NapiFileStatisticInfo> {
  const dirQueue: string[] = [directory];
  const allResults: NapiFileStatisticInfo[] = [];

  while (dirQueue.length > 0) {
    const currentDir = dirQueue.shift()!;
    const entries = await fs.promises.readdir(currentDir, { withFileTypes: true });
    const fileResults = await Promise.all(
      entries.
        map((entry) => {
          const fullPath = path.join(currentDir, entry.name);
          if (entry.isDirectory()) {
            dirQueue.push(fullPath);
            return null;
          } else if (isTargetFile(entry.name)) {
            return processFile(fullPath);
          }
          return null;
        }).
        filter(Boolean) as Promise<NapiFileStatisticInfo>[]
    );
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
    const [lines, napiCount] = await Promise.all([countLines(filePath), countNapiLines(filePath)]);

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
    const stat = await fs.promises.stat(directory);
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
