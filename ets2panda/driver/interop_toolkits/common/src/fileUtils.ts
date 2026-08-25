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

import path from 'node:path';

export enum Language {
  STATIC = '1.2',
  DYNAMIC = '1.1',
  HYBRID = 'hybrid',
}

export const TSBUILDINFO_EXT = '.tsbuildinfo';

export const INTERNAL_PREFIX = '?internal';

export enum Extension {
  ETS = '.ets',
  DETS = '.d.ets',
  TS = '.ts',
  DTS = '.d.ts',
  JS = '.js',
}

/** Normalize an input path to an absolute path using forward slashes. */
export function normalizePath(fileName: string): string {
  if (fileName.startsWith(INTERNAL_PREFIX)) {
    return fileName;
  }
  return path.resolve(fileName).replace(/\\/g, '/');
}

/** Convert all path separators to the current operating system separator. */
export function toPlatformPath(fileName: string): string {
  return fileName.replace(/[\\/]/g, path.sep);
}
