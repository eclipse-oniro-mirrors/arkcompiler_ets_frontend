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

import { promises as fs, type Dirent } from 'node:fs';
import * as path from 'node:path';

import {
  GlueGenDiagnosticError,
  GlueGenError,
  GlueGenErrorCode,
  GlueGenInternalError,
  errorMessage,
} from '../../errors';
import { LogData } from '../../logger';
import type { GlueGenContext } from '../../pipeline/context';
import type { StageScope } from '../../pipeline';
import { hasEtsSourceExtension, hasUseStaticDirectiveInFile } from '../../utils/staticSource';
import type { InteropTarget, MainModuleInfo, ModuleInfo } from '../configuration';
import { CONFIGURATION_ARTIFACT } from '../stageArtifacts';

type PrepareRequirements = readonly [typeof CONFIGURATION_ARTIFACT];

/** The file list path published by the prepare stage. */
export type InteropFileListPath = string & {
  readonly __interopFileListPath: 'InteropFileListPath';
};

export async function generateInteropFileList(
  scope: StageScope<GlueGenContext, PrepareRequirements>,
): Promise<InteropFileListPath> {
  const configuration = scope.get(CONFIGURATION_ARTIFACT);
  try {
    const files = await collectInteropFiles(configuration.interopTargets);
    return await writeInteropFileList(configuration.moduleTable.mainModule, files);
  } catch (error) {
    if (error instanceof GlueGenError) {
      throw error;
    }
    const data = new LogData({
      code: GlueGenErrorCode.GENERATE_INTEROP_FILE_LIST_FAIL,
      description: 'Gluegen could not generate the interop file list.',
      cause: errorMessage(error, 'unknown interop file-list failure'),
    });
    throw new GlueGenInternalError(data);
  }
}

async function collectInteropFiles(targets: ReadonlyMap<string, InteropTarget>): Promise<ReadonlySet<string>> {
  const files = new Set<string>();
  for (const target of targets.values()) {
    if (target.kind === 'items') {
      target.files.forEach((filePath) => files.add(filePath));
      continue;
    }
    for (const filePath of await scanPackage(target.moduleInfo)) {
      files.add(filePath);
    }
  }
  return files;
}

async function scanPackage(module: ModuleInfo): Promise<readonly string[]> {
  const files: string[] = [];
  for (const sourceRoot of module.sourceRoots) {
    await scanSourceRoot(sourceRoot, module, files);
  }
  return files;
}

async function scanSourceRoot(sourceRoot: string, module: ModuleInfo, files: string[]): Promise<void> {
  let stats;
  try {
    stats = await fs.stat(sourceRoot);
  } catch (error) {
    if (isNotFoundError(error)) {
      return;
    }
    throw unreadableSourceRoot(module.packageName, sourceRoot, error);
  }
  if (!stats.isDirectory()) {
    throw new GlueGenDiagnosticError(
      new LogData({
        code: GlueGenErrorCode.GENERATE_INTEROP_FILE_LIST_FAIL,
        description: `Source root for package "${module.packageName}" is not a directory.`,
        position: sourceRoot,
        solutions: ['Configure sourceRoots with a readable directory path.'],
        moreInfo: { packageName: module.packageName },
      }),
    );
  }
  await scanDirectory(sourceRoot, module, files);
}

async function scanDirectory(directoryPath: string, module: ModuleInfo, files: string[]): Promise<void> {
  let entries;
  try {
    entries = await fs.readdir(directoryPath, { withFileTypes: true });
  } catch (error) {
    throw unreadableSourceRoot(module.packageName, directoryPath, error);
  }

  for (const entry of entries) {
    await scanDirectoryEntry(directoryPath, entry, module, files);
  }
}

async function scanDirectoryEntry(
  directoryPath: string,
  entry: Dirent,
  module: ModuleInfo,
  files: string[],
): Promise<void> {
  const filePath = path.join(directoryPath, entry.name);
  if (entry.isDirectory()) {
    await scanDirectory(filePath, module, files);
    return;
  }
  if (!entry.isFile() || !hasEtsSourceExtension(filePath)) {
    return;
  }
  if (await isStaticFile(filePath, module)) {
    files.push(filePath);
  }
}

async function isStaticFile(filePath: string, module: ModuleInfo): Promise<boolean> {
  try {
    return await hasUseStaticDirectiveInFile(filePath);
  } catch (error) {
    throw new GlueGenDiagnosticError(
      new LogData({
        code: GlueGenErrorCode.GENERATE_INTEROP_FILE_LIST_FAIL,
        description: `A source file in package "${module.packageName}" cannot be inspected.`,
        cause: errorMessage(error, 'unknown interop file-list failure'),
        position: filePath,
        solutions: ['Check that the file is readable.'],
        moreInfo: { packageName: module.packageName },
      }),
    );
  }
}

async function writeInteropFileList(
  mainModule: MainModuleInfo,
  files: ReadonlySet<string>,
): Promise<InteropFileListPath> {
  const workspaceRoot = path.join(mainModule.cachePath, 'gluegen');
  const outputPath = path.join(workspaceRoot, 'fileInfo.txt');
  await fs.mkdir(workspaceRoot, { recursive: true });
  await fs.rm(outputPath, { force: true });
  await fs.writeFile(outputPath, [...files].join('\n'), 'utf8');
  return outputPath as InteropFileListPath;
}

function unreadableSourceRoot(packageName: string, sourceRoot: string, error: unknown): GlueGenDiagnosticError {
  return new GlueGenDiagnosticError(
    new LogData({
      code: GlueGenErrorCode.GENERATE_INTEROP_FILE_LIST_FAIL,
      description: `Source root for package "${packageName}" cannot be read.`,
      cause: errorMessage(error, 'unknown interop file-list failure'),
      position: sourceRoot,
      solutions: ['Check that the directory exists and is readable.'],
      moreInfo: { packageName },
    }),
  );
}

function isNotFoundError(error: unknown): boolean {
  return typeof error === 'object' && error !== null && 'code' in error && error.code === 'ENOENT';
}
