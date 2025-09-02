/*
 * Copyright (c) 2022-2024 Huawei Device Co., Ltd.
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
import * as ts from 'typescript';
import { forEachNodeInSubtree } from './ForEachNodeInSubtree';

export const COMMON_FILE_NAME = 'common.d.ts';
const commonApiInfoMap = new Set<ts.MethodDeclaration>();
function visitSourceFile(sf: ts.SourceFile | undefined): void {
  if (!sf) {
    return;
  }
  const callback = (node: ts.Node): void => {
    const isSave = ts.isMethodDeclaration(node) && ts.isClassDeclaration(node.parent);
    if (isSave) {
      commonApiInfoMap.add(node);
    }
  };
  forEachNodeInSubtree(sf, callback);
}
export function collectCommonApiInfo(tsProgram: ts.Program): void {
  const rootNames = tsProgram.getRootFileNames();
  rootNames.some((file) => {
    if (path.basename(file) === COMMON_FILE_NAME) {
      const commonSrcFile = tsProgram.getSourceFile(file);
      visitSourceFile(commonSrcFile);
      return true;
    }
    return false;
  });
}
export function getCommonApiInfoMap(): Set<ts.MethodDeclaration> | undefined {
  return commonApiInfoMap.size > 0 ? commonApiInfoMap : undefined;
}
