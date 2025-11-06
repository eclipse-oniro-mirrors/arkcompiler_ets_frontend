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

import fs from 'fs';
import path from 'path';
import { promisify } from 'util';
import { runWithIOLimit, mapWithLimit, FILES_PER_DIR_CONCURRENCY, DIRS_PER_DIR_CONCURRENCY } from './IoLimiter';

// Define file type extensions
const FILE_TYPES: { [key: string]: string[] } = {
  'C/C++': ['.c', '.h', '.cpp', '.hpp'],
  JavaScript: ['.js'],
  TypeScript: ['.ts'],
  JSON: ['.json', '.json5'],
  XML: ['.xml'],
  ArkTS: ['.ets'],
  'ArkTS Test': ['.test.ets']
};

// Comment regex patterns by file type
const COMMENT_REGEX1: { [key: string]: RegExp } = {
  'C/C++': /\/\/.*/g,
  JavaScript: /\/\/.*/g,
  TypeScript: /\/\/.*/g,
  ArkTS: /\/\/.*/g,
  'ArkTS Test': /\/\/.*/g,
  XML: /<!--.*?-->/g
};

const COMMENT_REGEX2: { [key: string]: RegExp } = {
  'C/C++': /\/\*[\s\S]*?\*\//g,
  JavaScript: /\/\*[\s\S]*?\*\//g,
  TypeScript: /\/\*[\s\S]*?\*\//g,
  ArkTS: /\/\*[\s\S]*?\*\//g,
  'ArkTS Test': /\/\*[\s\S]*?\*\//g
};

/**
 * Remove comments from file content
 */
function removeComments(content: string, fileType: string): string {
  if (COMMENT_REGEX1[fileType]) {
    content = content.replace(COMMENT_REGEX1[fileType], '');
  }
  if (COMMENT_REGEX2[fileType]) {
    content = content.replace(COMMENT_REGEX2[fileType], '');
  }
  return content;
}

/**
 * Count valid lines of code (excluding comments and empty lines)
 */
async function countLines(filePath: string, fileType: string): Promise<number> {
  try {
    const content = await promisify(fs.readFile)(filePath, 'utf-8');
    const cleanedContent = removeComments(content, fileType);
    const lines = cleanedContent.split('\n');

    // Filter out empty lines
    const validLines = lines.filter((line) => {
      return line.trim();
    });
    return validLines.length;
  } catch (error) {
    console.error(`Error reading ${filePath}: ${error}`);
    return 0;
  }
}

/**
 * Merge multiple result objects
 */
function mergeAllResults(results: { [key: string]: { fileCount: number; lineCount: number } }[]): {
  [key: string]: { fileCount: number; lineCount: number };
} {
  const combined: { [key: string]: { fileCount: number; lineCount: number } } = {};

  for (const result of results) {
    for (const [type, counts] of Object.entries(result)) {
      if (!combined[type]) {
        combined[type] = { fileCount: 0, lineCount: 0 };
      }
      combined[type].fileCount += counts.fileCount;
      combined[type].lineCount += counts.lineCount;
    }
  }

  return combined;
}

/**
 * Process directory entries recursively
 */
async function walkDir(dir: string): Promise<{ [key: string]: { fileCount: number; lineCount: number } }> {
  try {
    const entries = await runWithIOLimit(() => {
      return promisify(fs.readdir)(dir, { withFileTypes: true });
    });
    const fileResults = await processFiles(dir, entries);
    const dirResults = await processDirs(dir, entries);
    return mergeAllResults([fileResults, dirResults]);
  } catch (error) {
    console.error(`Error reading ${dir}: ${error}`);
    return {};
  }
}

/**
 * Process files in a directory
 */
async function processFiles(
  dir: string,
  entries: fs.Dirent[]
): Promise<{ [key: string]: { fileCount: number; lineCount: number } }> {
  const files = entries.filter((entry) => {
    return entry.isFile();
  });
  const fileResults = await mapWithLimit(files, FILES_PER_DIR_CONCURRENCY, async (entry) => {
    return processFileEntry(dir, entry);
  });
  return mergeAllResults(fileResults);
}

/**
 * Process a single file entry
 */
async function processFileEntry(
  dir: string,
  entry: fs.Dirent
): Promise<{ [key: string]: { fileCount: number; lineCount: number } }> {
  const fullPath = path.join(dir, entry.name);

  for (const fileType in FILE_TYPES) {
    const extensions = FILE_TYPES[fileType];
    const ext = path.extname(entry.name);

    if (extensions.includes(ext)) {
      const lines = await countLines(fullPath, fileType);
      return {
        [fileType]: {
          fileCount: 1,
          lineCount: lines
        }
      };
    }
  }

  return {};
}

/**
 * Process subdirectories recursively
 */
async function processDirs(
  dir: string,
  entries: fs.Dirent[]
): Promise<{ [key: string]: { fileCount: number; lineCount: number } }> {
  const dirEntries = entries.filter((entry) => {
    return entry.isDirectory();
  });
  const dirResults = await mapWithLimit(dirEntries, DIRS_PER_DIR_CONCURRENCY, async (entry) => {
    return walkDir(path.join(dir, entry.name));
  });
  return mergeAllResults(dirResults);
}

/**
 * Analyze directory and count files/lines by type
 */
async function analyzeDirectory(
  directory: string
): Promise<{ [key: string]: { fileCount: number; lineCount: number } }> {
  // Initialize results
  const results: { [key: string]: { fileCount: number; lineCount: number } } = {};
  for (const fileType in FILE_TYPES) {
    results[fileType] = { fileCount: 0, lineCount: 0 };
  }

  const finalResults = await walkDir(directory);
  Object.assign(results, finalResults);
  return results;
}

/**
 * Main export function
 */
export async function countFiles(
  directory: string
): Promise<{ [key: string]: { fileCount: number; lineCount: number } }> {
  try {
    const stats = await runWithIOLimit(() => {
      return promisify(fs.stat)(directory);
    });
    if (stats.isDirectory()) {
      return await analyzeDirectory(directory);
    }
    console.error('The directory is invalid!');
    return {};
  } catch (error) {
    console.error('Read directory failed', error);
    return {};
  }
}
