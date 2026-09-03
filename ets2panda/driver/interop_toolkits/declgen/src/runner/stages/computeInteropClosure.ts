/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

import * as path from 'node:path';
import * as common from '@interop-toolkits/common';
import { SentinelNotConfiguredError } from '../../errors';
import type * as dependencyResolver from 'dependency-resolver';
import type { Context } from '../context';
import {
  DEPENDENCY_GRAPH_ARTIFACT,
  INTEROP_CLOSURES_ARTIFACT,
  INTEROP_ENTRY_FILES_ARTIFACT,
  type InteropClosures,
  type InteropEntryFiles,
} from './stageArtifacts';

export function createComputeInteropClosureStage(): common.framework.pipeline.ProvidedStage<
  Context,
  readonly [typeof INTEROP_ENTRY_FILES_ARTIFACT, typeof DEPENDENCY_GRAPH_ARTIFACT],
  typeof INTEROP_CLOSURES_ARTIFACT
> {
  return common.framework.pipeline.Stage.start<Context>('compute-interop-closure')
    .requires(INTEROP_ENTRY_FILES_ARTIFACT, DEPENDENCY_GRAPH_ARTIFACT)
    .use('compute-interop-closure', {
      inputs: [],
      run: (scope): InteropClosures => {
        const entryFiles = scope.get(INTEROP_ENTRY_FILES_ARTIFACT);
        const dependencyGraph = scope.get(DEPENDENCY_GRAPH_ARTIFACT);
        validateSentinels(scope.context, entryFiles, dependencyGraph);
        return {
          staticClosure: getDeclgenInputClosure(
            entryFiles.staticEntryFiles,
            dependencyGraph,
            scope.context.fileManager,
          ),
          dynamicClosure: getDeclgenInputClosure(
            entryFiles.dynamicEntryFiles,
            dependencyGraph,
            scope.context.fileManager,
          ),
        };
      },
    })
    .provides(INTEROP_CLOSURES_ARTIFACT, {
      build: (_scope, outputs): InteropClosures => outputs['compute-interop-closure'],
    });
}

function validateSentinels(
  context: Context,
  entryFiles: InteropEntryFiles,
  dependencyGraph: dependencyResolver.DependencyGraph,
): void {
  const unconfiguredEntries: { fileName: string; error: SentinelNotConfiguredError }[] = [];
  for (const sentinel of dependencyGraph.getSentinels()) {
    if (
      !context.fileManager.isSourceFile(sentinel.fileName) ||
      entryFiles.staticEntryFiles.has(sentinel.fileName) ||
      entryFiles.dynamicEntryFiles.has(sentinel.fileName)
    ) {
      continue;
    }
    const fileMeta = context.fileManager.queryFileMeta(sentinel.fileName);
    if (fileMeta === undefined) {
      throw new common.errors.InternalError(`File meta for ${sentinel.fileName} is not found.`);
    }
    const moduleInfo = fileMeta.module;
    if (moduleInfo === undefined) {
      throw new common.errors.InternalError(`Module info for ${sentinel.fileName} is not found.`);
    }
    const language = fileMeta.language === common.fileUtils.Language.DYNAMIC ? 'Dynamic' : 'Static';
    const importerLanguage = language === 'Dynamic' ? 'static' : 'dynamic';
    const relativePath = path.relative(moduleInfo.modulePath, sentinel.fileName);
    unconfiguredEntries.push({
      fileName: sentinel.fileName,
      error: new SentinelNotConfiguredError({
        description: 'Failed to validate interop entries.',
        cause:
          `${language} file '${relativePath}' of package ${moduleInfo.packageName} ` +
          `is imported by some ${importerLanguage} files. ` +
          'But it is not configured as an interop entry.',
        solutions: [`Add it into the interop configuration.`],
      }),
    });
  }
  if (unconfiguredEntries.length > 0) {
    unconfiguredEntries.sort((left, right) => comparePath(left.fileName, right.fileName));
    throw new common.errors.AggregateUserError(unconfiguredEntries.map((entry) => entry.error));
  }
}

function comparePath(left: string, right: string): number {
  if (left === right) {
    return 0;
  }
  return left < right ? -1 : 1;
}

function getDeclgenInputClosure(
  entryFiles: Set<string>,
  dependencyGraph: dependencyResolver.DependencyGraph,
  fileManager: common.fileManager.FileManager,
): Set<string> {
  const closure = new Set<string>();
  for (const entryFile of entryFiles) {
    if (!fileManager.isSourceFile(entryFile)) {
      continue;
    }
    closure.add(entryFile);
    const reachable = dependencyGraph.getPartialDependencyChain(entryFile, (node) =>
      fileManager.isSourceFile(node.fileName),
    );
    for (const file of reachable) {
      closure.add(file);
    }
  }
  return closure;
}
