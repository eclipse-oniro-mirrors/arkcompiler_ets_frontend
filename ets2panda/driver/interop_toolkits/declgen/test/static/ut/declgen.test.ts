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

import * as fs from 'node:fs';
import * as os from 'node:os';
import * as path from 'node:path';

import * as common from '@interop-toolkits/common';

import type { BuildConfig } from '../../../src/buildConfig';
import { StaticDeclgen } from '../../../src/static/declgen/declgen';
import { buildDeclgenOutputPath } from '../../../src/static/declgen/executor';
import { Graph, GraphNode } from '../../../src/static/declgen/graph';
import type { DeclgenTask } from '../../../src/static/declgen/task';

interface DeclFileInfo {
  delFilePath: string;
  declLastModified: number | null;
  sourceFilePath: string;
  sourceFileLastModified: number | null;
}

interface StaticDeclgenInternals {
  buildGraph: Graph<DeclgenTask>;
  filesToGenerate: string[];
  declFileMap: Map<string, DeclFileInfo>;
  shouldRunSerial(): boolean;
  createIncrementalGraph(graph: Graph<{ inputFiles: string[] }>): Graph<DeclgenTask>;
  backupDeclgenFiles(): Promise<void>;
}

function createBuildConfig(root: string, sourceFile: string, outputRoot: string): BuildConfig {
  return {
    plugins: {},
    buildSdkPath: '',
    buildDynamicSdkPath: '',
    dynamicPlugins: {},
    buildMode: 'Debug',
    buildType: 'BUILD',
    projectRootPath: root,
    cachePath: path.join(root, 'cache'),
    compileSdkVersion: 1,
    compatibleSdkVersion: 1,
    bundleName: 'bundle',
    moduleName: 'module',
    packageName: 'package',
    dependentModuleList: [
      {
        packageName: 'package',
        modulePath: root,
        sourceRoots: [root],
        entryFile: sourceFile,
        declgenV1OutPath: outputRoot,
        declgenV2OutPath: path.join(root, 'interop-declarations'),
        staticFiles: [sourceFile],
        dynamicFiles: [],
      },
    ],
    sdkAliasMap: {},
    hasMainModule: true,
    modulePath: root,
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

function asInternals(declgen: StaticDeclgen): StaticDeclgenInternals {
  return declgen as unknown as StaticDeclgenInternals;
}

describe('StaticDeclgen', () => {
  let root: string;
  let sourceFile: string;
  let outputRoot: string;
  let arktsconfigPath: string;
  let buildConfig: BuildConfig;

  beforeEach(() => {
    root = fs.mkdtempSync(path.join(os.tmpdir(), 'static-declgen-test-'));
    sourceFile = path.join(root, 'src', 'main.ets');
    outputRoot = path.join(root, 'declarations');
    arktsconfigPath = path.join(root, 'cache', 'arktsconfig.json');
    fs.mkdirSync(path.dirname(sourceFile), { recursive: true });
    fs.writeFileSync(sourceFile, 'export const value = 1;');
    buildConfig = createBuildConfig(root, sourceFile, outputRoot);
    delete process.env.STATIC_DECLGEN_FORCE_NO_PARALLEL;
  });

  afterEach(() => {
    delete process.env.STATIC_DECLGEN_FORCE_NO_PARALLEL;
    fs.rmSync(root, { recursive: true, force: true });
  });

  it('uses parallel mode by default', () => {
    const internals = asInternals(new StaticDeclgen(buildConfig, [sourceFile], arktsconfigPath));
    expect(internals.shouldRunSerial()).toBe(false);
  });

  it('uses serial mode when STATIC_DECLGEN_FORCE_NO_PARALLEL is true', () => {
    const internals = asInternals(new StaticDeclgen(buildConfig, [sourceFile], arktsconfigPath));
    internals.buildGraph = Graph.createGraphFromNodes([
      new GraphNode('first', { inputFiles: [sourceFile], arktsconfigPath: 'arktsconfig', buildConfig }),
      new GraphNode('second', { inputFiles: [sourceFile], arktsconfigPath: 'arktsconfig', buildConfig }),
    ]);
    process.env.STATIC_DECLGEN_FORCE_NO_PARALLEL = 'true';

    expect(internals.shouldRunSerial()).toBe(true);
  });

  it('removes up-to-date clusters and prunes their graph edges', () => {
    const secondSource = path.join(root, 'src', 'second.ets');
    fs.writeFileSync(secondSource, 'export const second = 2;');
    const declgen = new StaticDeclgen(buildConfig, [sourceFile], arktsconfigPath);
    const internals = asInternals(declgen);
    const outputPath = buildDeclgenOutputPath(
      sourceFile,
      {
        language: common.fileUtils.Language.STATIC,
        moduleName: 'module',
        modulePath: root,
        packageName: 'package',
        packageVersion: '',
        dynamicFiles: new Set(),
        staticFiles: new Set([sourceFile]),
        declgenV1OutPath: outputRoot,
        declgenV2OutPath: '',
      },
      buildConfig.cachePath,
    ).declEtsOutputPath;
    fs.writeFileSync(outputPath, 'declaration');
    internals.declFileMap.set(sourceFile, {
      delFilePath: outputPath,
      declLastModified: fs.statSync(outputPath).mtimeMs,
      sourceFilePath: sourceFile,
      sourceFileLastModified: fs.statSync(sourceFile).mtimeMs,
    });
    const first = new GraphNode('first', { inputFiles: [sourceFile] });
    const second = new GraphNode('second', { inputFiles: [secondSource] });
    first.descendants.add(second.id);
    second.predecessors.add(first.id);

    const graph = internals.createIncrementalGraph(Graph.createGraphFromNodes([first, second]));

    expect([...graph.nodes].map((node) => node.id)).toEqual(['second']);
    expect([...graph.nodes][0].predecessors.size).toBe(0);
  });

  it('backs up a declaration modified after the previous generation', async () => {
    const declgen = new StaticDeclgen(buildConfig, [sourceFile], arktsconfigPath);
    const internals = asInternals(declgen);
    const moduleInfo = {
      language: common.fileUtils.Language.STATIC,
      moduleName: 'module',
      modulePath: root,
      packageName: 'package',
      packageVersion: '',
      dynamicFiles: new Set<string>(),
      staticFiles: new Set([sourceFile]),
      declgenV1OutPath: outputRoot,
      declgenV2OutPath: '',
    };
    const outputPath = buildDeclgenOutputPath(sourceFile, moduleInfo, buildConfig.cachePath).declEtsOutputPath;
    fs.writeFileSync(outputPath, 'user modification');
    internals.filesToGenerate = [sourceFile];
    internals.declFileMap.set(sourceFile, {
      delFilePath: outputPath,
      declLastModified: fs.statSync(outputPath).mtimeMs - 1000,
      sourceFilePath: sourceFile,
      sourceFileLastModified: fs.statSync(sourceFile).mtimeMs - 1000,
    });

    await internals.backupDeclgenFiles();

    expect(fs.readFileSync(`${outputPath}.backup`, 'utf-8')).toBe('user modification');
  });
});
