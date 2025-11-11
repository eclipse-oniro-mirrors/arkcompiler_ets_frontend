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

export function getFileLanguageVersion(fileSource: string): string {
  const lines = fileSource.split(/\r?\n/);
  if (lines.length === 0) {
    return LANGUAGE_VERSION.ARKTS_1_1;
  }
  const firstLine = lines[0].trim();
  if (firstLine === "'use static'" || firstLine === "'use static';") {
    return LANGUAGE_VERSION.ARKTS_1_2;
  }
  return LANGUAGE_VERSION.ARKTS_1_1;
}
