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
import * as dependencyResolver from 'dependency-resolver';

import type { BuildConfig } from '../../src/buildConfig';
import { SentinelNotConfiguredError } from '../../src/errors';
import type { Context } from '../../src/runner/context';
import { createComputeInteropClosureStage } from '../../src/runner/stages/computeInteropClosure';
import {
  DEPENDENCY_GRAPH_ARTIFACT,
  INTEROP_ENTRY_FILES_ARTIFACT,
  type InteropEntryFiles,
} from '../../src/runner/stages/stageArtifacts';

type ComputeInteropClosureScope = Parameters<ReturnType<typeof createComputeInteropClosureStage>['run']>[0];

const PACKAGE_NAME = 'test-package';

describe('computeInteropClosure stage', () => {
  let projectRoot: string;
  let modulePath: string;
  let staticFile: string;
  let dynamicFile: string;
  let secondStaticFile: string;
  let context: Context;

  beforeEach(() => {
    projectRoot = common.fileUtils.normalizePath('/declgen-compute-interop-closure-test/project');
    modulePath = path.join(projectRoot, 'entry');
    staticFile = common.fileUtils.normalizePath(path.join(modulePath, 'src', 'main', 'ets', 'static.ets'));
    dynamicFile = common.fileUtils.normalizePath(path.join(modulePath, 'src', 'main', 'ets', 'dynamic.ts'));
    secondStaticFile = common.fileUtils.normalizePath(path.join(modulePath, 'src', 'main', 'ets', 'b.ets'));
    context = {
      buildConfig: createBuildConfig(projectRoot, modulePath),
      fileManager: createFileManager(),
      tsconfigPath: '',
      arktsconfigPath: '',
    };
  });

  it('aggregates an unconfigured dynamic sentinel into one user error block', async () => {
    const entryFiles = createEntryFiles();
    const graph = createDependencyGraph([[dynamicFile, dependencyResolver.NodeType.DYNAMIC]]);

    const error = await captureStageError(entryFiles, graph);

    expect(error.errors).toHaveLength(1);
    expect(error.errors[0]).toBeInstanceOf(SentinelNotConfiguredError);
    expect(error.errors[0].errorMessage.description).toBe('Failed to validate interop entries.');
    expect(error.errors[0].errorMessage.cause).toBe(
      `Dynamic file '${path.relative(modulePath, dynamicFile)}' of package ${PACKAGE_NAME} ` +
        'is imported by some static files. But it is not configured as an interop entry.',
    );
    expect(error.errors[0].errorMessage.solutions).toEqual(['Add it into the interop configuration.']);
  });

  it('aggregates an unconfigured static sentinel into one user error block', async () => {
    const entryFiles = createEntryFiles();
    const graph = createDependencyGraph([[staticFile, dependencyResolver.NodeType.STATIC]]);

    const error = await captureStageError(entryFiles, graph);

    expect(error.errors).toHaveLength(1);
    expect(error.errors[0].errorMessage.cause).toBe(
      `Static file '${path.relative(modulePath, staticFile)}' of package ${PACKAGE_NAME} ` +
        'is imported by some dynamic files. But it is not configured as an interop entry.',
    );
  });

  it('reports every unconfigured sentinel as an independent error block in path order', async () => {
    const entryFiles = createEntryFiles();
    const graph = createDependencyGraph([
      [dynamicFile, dependencyResolver.NodeType.DYNAMIC],
      [staticFile, dependencyResolver.NodeType.STATIC],
      [secondStaticFile, dependencyResolver.NodeType.STATIC],
    ]);

    const error = await captureStageError(entryFiles, graph);

    expect(error.errors).toHaveLength(3);
    const causes = error.errors.map((innerError) => innerError.errorMessage.cause);
    expect(causes).toEqual([
      `Static file '${path.relative(modulePath, secondStaticFile)}' of package ${PACKAGE_NAME} ` +
        'is imported by some dynamic files. But it is not configured as an interop entry.',
      `Dynamic file '${path.relative(modulePath, dynamicFile)}' of package ${PACKAGE_NAME} ` +
        'is imported by some static files. But it is not configured as an interop entry.',
      `Static file '${path.relative(modulePath, staticFile)}' of package ${PACKAGE_NAME} ` +
        'is imported by some dynamic files. But it is not configured as an interop entry.',
    ]);
    for (const innerError of error.errors) {
      expect(innerError.errorMessage.description).toBe('Failed to validate interop entries.');
      expect(innerError.errorMessage.solutions).toEqual(['Add it into the interop configuration.']);
    }
  });

  it('skips sentinels that are configured as interop entries', async () => {
    const entryFiles = createEntryFiles(dynamicFile);
    const graph = createDependencyGraph([
      [dynamicFile, dependencyResolver.NodeType.DYNAMIC],
      [secondStaticFile, dependencyResolver.NodeType.STATIC],
    ]);

    const error = await captureStageError(entryFiles, graph);

    expect(error.errors).toHaveLength(1);
    expect(error.errors[0].errorMessage.cause).toContain('b.ets');
  });

  it('accepts a sentinel that is configured as an interop entry', async () => {
    const entryFiles = createEntryFiles(dynamicFile);
    const graph = createDependencyGraph([[dynamicFile, dependencyResolver.NodeType.DYNAMIC]]);

    const closures = await runStage(context, entryFiles, graph);
    expect([...closures.dynamicClosure]).toEqual([dynamicFile]);
    expect([...closures.staticClosure]).toEqual([]);
  });

  function createFileManager(): common.fileManager.FileManager {
    return new common.fileManager.FileManagerBuilder()
      .addModuleList([
        {
          packageName: PACKAGE_NAME,
          modulePath,
          staticFiles: [staticFile, secondStaticFile],
          dynamicFiles: [dynamicFile],
        },
      ])
      .build();
  }

  function createEntryFiles(...configuredSentinels: readonly string[]): InteropEntryFiles {
    return {
      staticEntryFiles: new Set(configuredSentinels.filter((file) => context.fileManager.isStaticSourceFile(file))),
      dynamicEntryFiles: new Set(configuredSentinels.filter((file) => context.fileManager.isDynamicSourceFile(file))),
    };
  }

  function createDependencyGraph(
    sentinels: readonly (readonly [string, dependencyResolver.NodeType])[],
  ): dependencyResolver.DependencyGraph {
    const nodes = new Map<string, dependencyResolver.DependencyNode>(
      sentinels.map(([fileName, type]) => [
        fileName,
        {
          fileName,
          type,
          isSentinel: true,
          isResolved: false,
          dependencies: [],
          dependants: [],
        },
      ]),
    );
    return new dependencyResolver.DependencyGraph(nodes);
  }

  function createStageScope(
    entryFiles: InteropEntryFiles,
    graph: dependencyResolver.DependencyGraph,
  ): ComputeInteropClosureScope {
    const artifacts = new Map<string, unknown>([
      [INTEROP_ENTRY_FILES_ARTIFACT.name, entryFiles],
      [DEPENDENCY_GRAPH_ARTIFACT.name, graph],
    ]);
    return {
      context,
      get: (artifact: { readonly name: string }): unknown => {
        const value = artifacts.get(artifact.name);
        if (value === undefined) {
          throw new Error(`required pipeline artifact is unavailable: ${artifact.name}`);
        }
        return value;
      },
    } as unknown as ComputeInteropClosureScope;
  }

  function runStage(
    context: Context,
    entryFiles: InteropEntryFiles,
    graph: dependencyResolver.DependencyGraph,
  ): Promise<{ staticClosure: Set<string>; dynamicClosure: Set<string> }> {
    return createComputeInteropClosureStage().run(createStageScope(entryFiles, graph));
  }

  async function captureStageError(
    entryFiles: InteropEntryFiles,
    graph: dependencyResolver.DependencyGraph,
  ): Promise<common.errors.AggregateUserError> {
    try {
      await runStage(context, entryFiles, graph);
    } catch (error) {
      expect(error).toBeInstanceOf(common.errors.AggregateUserError);
      return error as common.errors.AggregateUserError;
    }
    throw new Error('Expected compute-interop-closure stage to throw AggregateUserError.');
  }
});

function createBuildConfig(projectRootPath: string, modulePath: string): BuildConfig {
  return {
    plugins: {},
    buildSdkPath: '',
    buildDynamicSdkPath: '',
    dynamicPlugins: {},
    buildMode: 'Debug',
    buildType: 'BUILD',
    projectRootPath,
    cachePath: path.join(projectRootPath, 'cache'),
    compileSdkVersion: 1,
    compatibleSdkVersion: 1,
    bundleName: 'bundle',
    moduleName: 'entry',
    packageName: PACKAGE_NAME,
    dependentModuleList: [
      {
        packageName: PACKAGE_NAME,
        modulePath,
        sourceRoots: [modulePath],
        entryFile: '',
        staticFiles: [],
        dynamicFiles: [],
      },
    ],
    sdkAliasMap: {},
    hasMainModule: true,
    modulePath,
    byteCodeHar: false,
    declgenBridgeConfigPath: '',
    interopConfigPath: '',
    externalApiPaths: [],
    pandaSdkPath: '',
    sdkPaths: {
      staticSdkPaths: [],
      dynamicSdkPaths: [],
      staticInteropSdkPaths: [],
      dynamicInteropSdkPaths: [],
    },
  };
}
