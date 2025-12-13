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
import * as os from 'os';
import { DECL_ETS_SUFFIX, LANGUAGE_VERSION } from './preDefine';

export function throwError(error: string): never {
  throw new Error(error);
}

export function withWarning<T>(value: T, message: string): T {
  console.warn(message);
  return value;
}

export function changeFileExtension(file: string, targetExt: string, originExt = ''): string {
  let currentExt = originExt.length === 0 ? path.extname(file) : originExt;
  let fileWithoutExt = file.substring(0, file.lastIndexOf(currentExt));
  return fileWithoutExt + targetExt;
}

export function ensurePathExists(filePath: string): void {
  try {
    const dirPath: string = path.dirname(filePath);
    if (!fs.existsSync(dirPath)) {
      fs.mkdirSync(dirPath, { recursive: true });
    }
  } catch (error) {
    if (error instanceof Error) {
      console.error(`Error: ${error.message}`);
    }
  }
}

export function isMac(): boolean {
  return os.type() === 'Darwin';
}

export function changeDeclgenFileExtension(file: string, targetExt: string): string {
  if (file.endsWith(DECL_ETS_SUFFIX)) {
    return changeFileExtension(file, targetExt, DECL_ETS_SUFFIX);
  }
  return changeFileExtension(file, targetExt);
}

export function getModuleNameAndPath(filePath: string, projectPath: string): [string, string] {
  let moduleName: string = '';
  let moduleRootPath: string = '';
  if (filePath.indexOf(projectPath) >= 0) {
    const relativePath = path.relative(projectPath, filePath);
    moduleName = relativePath.split(path.sep)[0];
    moduleRootPath = path.join(projectPath, moduleName);
  }
  return [moduleName, moduleRootPath];
}

// Skip comment, check whether the first valid line contains 'use static'.
export function getFileLanguageVersion(fileSource: string): string {
  const lines = fileSource.split('\n');
  let inMultiLineComment = false;
  let effectiveLine = '';

  for (let i = 0; i < lines.length; i++) {
    let line = lines[i];

    if (inMultiLineComment) {
      const endIndex = line.indexOf('*/');
      if (endIndex !== -1) {
        line = line.substring(endIndex + 2);
        inMultiLineComment = false;
      } else {
        continue;
      }
    }

    const singleLineIndex = line.indexOf('//');
    if (singleLineIndex !== -1) {
      line = line.substring(0, singleLineIndex);
    }

    const multiLineStart = line.indexOf('/*');
    if (multiLineStart !== -1) {
      const multiLineEnd = line.indexOf('*/', multiLineStart + 2);
      if (multiLineEnd !== -1) {
        line = line.substring(0, multiLineStart) + line.substring(multiLineEnd + 2);
      } else {
        line = line.substring(0, multiLineStart);
        inMultiLineComment = true;
      }
    }

    const trimmedLine = line.trim();
    if (trimmedLine === '') {
      continue;
    }

    effectiveLine = trimmedLine;
    break;
  }

  if (effectiveLine.includes('use static')) {
    return LANGUAGE_VERSION.ARKTS_1_2;
  }

  return LANGUAGE_VERSION.ARKTS_1_1;
}
