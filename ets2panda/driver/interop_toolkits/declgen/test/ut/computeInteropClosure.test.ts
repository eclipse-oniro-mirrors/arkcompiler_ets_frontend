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
const DEPENDENCY_PACKAGE_NAME = 'dependency-library';

describe('computeInteropClosure stage', () => {
  let projectRoot: string;
  let modulePath: string;
  let staticFile: string;
  let dynamicFile: string;
  let secondStaticFile: string;
  let dependencyModulePath: string;
  let dependencyStaticFile: string;
  let dependencyDynamicFile: string;
  let context: Context;

  beforeEach(() => {
    projectRoot = common.fileUtils.normalizePath('/declgen-compute-interop-closure-test/project');
    modulePath = path.join(projectRoot, 'entry');
    staticFile = common.fileUtils.normalizePath(path.join(modulePath, 'src', 'main', 'ets', 'static.ets'));
    dynamicFile = common.fileUtils.normalizePath(path.join(modulePath, 'src', 'main', 'ets', 'dynamic.ts'));
    secondStaticFile = common.fileUtils.normalizePath(path.join(modulePath, 'src', 'main', 'ets', 'b.ets'));
    dependencyModulePath = common.fileUtils.normalizePath(path.join(projectRoot, DEPENDENCY_PACKAGE_NAME));
    dependencyStaticFile = common.fileUtils.normalizePath(
      path.join(dependencyModulePath, 'src', 'main', 'ets', 'static.ets'),
    );
    dependencyDynamicFile = common.fileUtils.normalizePath(
      path.join(dependencyModulePath, 'src', 'main', 'ets', 'dynamic.ts'),
    );
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

  it('skips a bytecode-HAR-module sentinel whose opposite-language importers are all in bytecode modules', async () => {
    setupDependencyModuleContext({ byteCodeHar: true });
    const entryFiles = createEntryFiles();
    const graph = createDependencyGraph([
      [dependencyDynamicFile, dependencyResolver.NodeType.DYNAMIC, [dependencyStaticFile]],
    ]);

    const closures = await runStage(context, entryFiles, graph);

    expect([...closures.staticClosure]).toEqual([]);
    expect([...closures.dynamicClosure]).toEqual([]);
  });

  it('skips an HSP-module sentinel whose opposite-language importers are all in HSP modules', async () => {
    setupDependencyModuleContext({ moduleType: 'shared', byteCodeHar: false });
    const entryFiles = createEntryFiles();
    const graph = createDependencyGraph([
      [dependencyDynamicFile, dependencyResolver.NodeType.DYNAMIC, [dependencyStaticFile]],
    ]);

    const closures = await runStage(context, entryFiles, graph);

    expect([...closures.staticClosure]).toEqual([]);
    expect([...closures.dynamicClosure]).toEqual([]);
  });

  it('reports a source-HAR-module sentinel whose opposite-language importers are all in that module', async () => {
    setupDependencyModuleContext({ moduleType: 'har' });
    const entryFiles = createEntryFiles();
    const graph = createDependencyGraph([
      [dependencyDynamicFile, dependencyResolver.NodeType.DYNAMIC, [dependencyStaticFile]],
    ]);

    const error = await captureStageError(entryFiles, graph);

    expect(error.errors).toHaveLength(1);
    expect(error.errors[0].errorMessage.cause).toBe(
      `Dynamic file '${path.relative(dependencyModulePath, dependencyDynamicFile)}' of package ${DEPENDENCY_PACKAGE_NAME} ` +
        'is imported by some static files. But it is not configured as an interop entry.',
    );
  });

  it('reports a source-HSP-module sentinel when the byteCodeHar field is absent', async () => {
    setupDependencyModuleContext({ moduleType: 'shared' });
    const entryFiles = createEntryFiles();
    const graph = createDependencyGraph([
      [dependencyDynamicFile, dependencyResolver.NodeType.DYNAMIC, [dependencyStaticFile]],
    ]);

    const error = await captureStageError(entryFiles, graph);

    expect(error.errors).toHaveLength(1);
    expect(error.errors[0].errorMessage.cause).toBe(
      `Dynamic file '${path.relative(dependencyModulePath, dependencyDynamicFile)}' of package ${DEPENDENCY_PACKAGE_NAME} ` +
        'is imported by some static files. But it is not configured as an interop entry.',
    );
  });

  it('reports a bytecode-module sentinel when an importer comes from a source module', async () => {
    setupDependencyModuleContext({ byteCodeHar: true });
    const entryFiles = createEntryFiles();
    const graph = createDependencyGraph([
      [dependencyDynamicFile, dependencyResolver.NodeType.DYNAMIC, [dependencyStaticFile, staticFile]],
    ]);

    const error = await captureStageError(entryFiles, graph);

    expect(error.errors).toHaveLength(1);
    expect(error.errors[0].errorMessage.cause).toBe(
      `Dynamic file '${path.relative(dependencyModulePath, dependencyDynamicFile)}' of package ${DEPENDENCY_PACKAGE_NAME} ` +
        'is imported by some static files. But it is not configured as an interop entry.',
    );
    const expectedImporters = [dependencyStaticFile, staticFile].sort();
    expect(error.errors[0].errorMessage.moreInfo).toEqual({
      'imported by': `\n${expectedImporters.map((importer) => `      ${importer}`).join('\n')}`,
    });
  });

  it('skips a bytecode-module sentinel that has no opposite-language importers', async () => {
    setupDependencyModuleContext({ byteCodeHar: true });
    const entryFiles = createEntryFiles();
    const graph = createDependencyGraph([[dependencyDynamicFile, dependencyResolver.NodeType.DYNAMIC]]);

    const closures = await runStage(context, entryFiles, graph);

    expect([...closures.staticClosure]).toEqual([]);
    expect([...closures.dynamicClosure]).toEqual([]);
  });

  it('reports a source-module sentinel whose importers are all in bytecode modules', async () => {
    setupDependencyModuleContext({ byteCodeHar: true });
    const entryFiles = createEntryFiles();
    const graph = createDependencyGraph([[dynamicFile, dependencyResolver.NodeType.DYNAMIC, [dependencyStaticFile]]]);

    const error = await captureStageError(entryFiles, graph);

    expect(error.errors).toHaveLength(1);
    expect(error.errors[0].errorMessage.cause).toBe(
      `Dynamic file '${path.relative(modulePath, dynamicFile)}' of package ${PACKAGE_NAME} ` +
        'is imported by some static files. But it is not configured as an interop entry.',
    );
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

  /** Registers an extra dependency module with the given module flags next to the default source module. */
  function setupDependencyModuleContext(moduleConfig: { moduleType?: string; byteCodeHar?: boolean }): void {
    context = {
      ...context,
      buildConfig: createBuildConfig(projectRoot, modulePath, [
        {
          packageName: DEPENDENCY_PACKAGE_NAME,
          modulePath: dependencyModulePath,
          sourceRoots: [dependencyModulePath],
          entryFile: '',
          ...(moduleConfig.moduleType === undefined ? {} : { moduleType: moduleConfig.moduleType }),
          ...(moduleConfig.byteCodeHar === undefined ? {} : { byteCodeHar: moduleConfig.byteCodeHar }),
          staticFiles: [dependencyStaticFile],
          dynamicFiles: [dependencyDynamicFile],
        },
      ]),
      fileManager: new common.fileManager.FileManagerBuilder()
        .addModuleList([
          {
            packageName: PACKAGE_NAME,
            modulePath,
            staticFiles: [staticFile, secondStaticFile],
            dynamicFiles: [dynamicFile],
          },
          {
            packageName: DEPENDENCY_PACKAGE_NAME,
            modulePath: dependencyModulePath,
            staticFiles: [dependencyStaticFile],
            dynamicFiles: [dependencyDynamicFile],
          },
        ])
        .build(),
    };
  }

  function createEntryFiles(...configuredSentinels: readonly string[]): InteropEntryFiles {
    return {
      staticEntryFiles: new Set(configuredSentinels.filter((file) => context.fileManager.isStaticSourceFile(file))),
      dynamicEntryFiles: new Set(configuredSentinels.filter((file) => context.fileManager.isDynamicSourceFile(file))),
    };
  }

  function createDependencyGraph(
    sentinels: readonly (
      | readonly [string, dependencyResolver.NodeType]
      | readonly [string, dependencyResolver.NodeType, readonly string[]]
    )[],
  ): dependencyResolver.DependencyGraph {
    const nodes = new Map<string, dependencyResolver.DependencyNode>(
      sentinels.map(([fileName, type, dependants]) => [
        fileName,
        {
          fileName,
          type,
          isSentinel: true,
          isResolved: false,
          dependencies: [],
          dependants: [...(dependants ?? [])],
        },
      ]),
    );
    return new dependencyResolver.DependencyGraph(nodes);
  }

  it('lists the importing files in the sentinel error more info', async () => {
    const entryFiles = createEntryFiles();
    const graph = createDependencyGraph([
      [dynamicFile, dependencyResolver.NodeType.DYNAMIC, [staticFile, secondStaticFile]],
    ]);

    const error = await captureStageError(entryFiles, graph);

    expect(error.errors).toHaveLength(1);
    expect(error.errors[0].errorMessage.cause).toBe(
      `Dynamic file '${path.relative(modulePath, dynamicFile)}' of package ${PACKAGE_NAME} ` +
        'is imported by some static files. ' +
        'But it is not configured as an interop entry.',
    );
    expect(error.errors[0].errorMessage.moreInfo).toEqual({
      'imported by': `\n      ${secondStaticFile}\n      ${staticFile}`,
    });
  });

  it('lists only importers of the opposite language in more info', async () => {
    const entryFiles = createEntryFiles();
    const graph = createDependencyGraph([
      [staticFile, dependencyResolver.NodeType.STATIC, [dynamicFile, secondStaticFile]],
    ]);

    const error = await captureStageError(entryFiles, graph);

    expect(error.errors).toHaveLength(1);
    expect(error.errors[0].errorMessage.cause).toBe(
      `Static file '${path.relative(modulePath, staticFile)}' of package ${PACKAGE_NAME} ` +
        'is imported by some dynamic files. ' +
        'But it is not configured as an interop entry.',
    );
    expect(error.errors[0].errorMessage.moreInfo).toEqual({
      'imported by': `\n      ${dynamicFile}`,
    });
  });

  it('lists all importers in more info without truncation', async () => {
    const staticImporters = Array.from({ length: 7 }, (_, index) =>
      common.fileUtils.normalizePath(path.join(modulePath, 'src', `importer${index}.ets`)),
    );
    context = {
      ...context,
      fileManager: new common.fileManager.FileManagerBuilder()
        .addModuleList([
          {
            packageName: PACKAGE_NAME,
            modulePath,
            staticFiles: staticImporters,
            dynamicFiles: [dynamicFile],
          },
        ])
        .build(),
    };
    const entryFiles = createEntryFiles();
    const graph = createDependencyGraph([[dynamicFile, dependencyResolver.NodeType.DYNAMIC, staticImporters]]);

    const error = await captureStageError(entryFiles, graph);

    expect(error.errors[0].errorMessage.moreInfo).toEqual({
      'imported by': `\n${staticImporters.map((importer) => `      ${importer}`).join('\n')}`,
    });
  });

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

function createBuildConfig(
  projectRootPath: string,
  modulePath: string,
  dependentModules: BuildConfig['dependentModuleList'] = [],
): BuildConfig {
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
      ...dependentModules,
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
