/*
 * Copyright (c) 2022-2025 Huawei Device Co., Ltd.
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

import type * as ts from 'typescript';
import * as path from 'node:path';
import { STANDARD_LIBRARIES, SDK_MODULES } from '../consts/StandardLibraries';

export function isStdLibraryType(type: ts.Type): boolean {
  return isStdLibrarySymbol(type.aliasSymbol ?? type.getSymbol());
}

export function isStdLibrarySymbol(sym: ts.Symbol | undefined, checkSdkLibs = false): boolean {
  if (sym?.declarations && sym.declarations.length > 0) {
    const srcFile = sym.declarations[0].getSourceFile();
    return (
      srcFile && STANDARD_LIBRARIES.includes(path.basename(srcFile.fileName).toLowerCase()) ||
      checkSdkLibs && SDK_MODULES.includes(path.basename(srcFile.fileName).toLowerCase())
    );
  }
  return false;
}
