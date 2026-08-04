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

import { promises as fs } from 'node:fs';
import { tmpdir } from 'node:os';
import * as path from 'node:path';

import type { BuildConfig } from '../../src/contracts';
import { GlueGenDiagnosticError } from '../../src/errors';
import type { ILogger } from '../../src/logger';
import { Pipeline } from '../../src/pipeline';
import type { GlueGenContext } from '../../src/pipeline/context';
import { createConfigurationStage } from '../../src/stages/configuration';
import { createPrepareStage } from '../../src/stages/prepare';

const SILENT_LOGGER: ILogger = {
  printInfo: () => undefined,
  printWarn: () => undefined,
  printDebug: () => undefined,
  printError: () => undefined,
  printErrorAndExit: () => undefined,
};

describe('interop preparation', () => {
  it('resolves static targets, skips missing and module-root source roots, and rebuilds the file list', async () => {
    const projectRoot = await fs.mkdtemp(path.join(tmpdir(), 'gluegen-interop-'));
    try {
      const entryRoot = path.join(projectRoot, 'entry');
      const libraryRoot = path.join(projectRoot, 'library');
      const librarySource = path.join(libraryRoot, 'src');
      const dynamicLibraryRoot = path.join(projectRoot, 'dynamic-library');
      const leafRoot = path.join(projectRoot, 'leaf');
      const leafSource = path.join(leafRoot, 'src');
      const entryFile = path.join(entryRoot, 'Index.ets');
      const libraryEntryFile = path.join(libraryRoot, 'Index.ets');
      const dynamicLibraryEntryFile = path.join(dynamicLibraryRoot, 'Index.ets');
      const libraryFile = path.join(librarySource, 'Library.ets');
      const declarationFile = path.join(librarySource, 'Library.d.ets');
      const dynamicFile = path.join(librarySource, 'Dynamic.ets');
      const generatedFile = path.join(libraryRoot, 'build', 'Generated.ets');
      const selectedLeafFile = path.join(leafSource, 'Selected.ets');
      const ownLeafFile = path.join(leafSource, 'Own.ets');
      await fs.mkdir(entryRoot, { recursive: true });
      await fs.mkdir(librarySource, { recursive: true });
      await fs.mkdir(dynamicLibraryRoot, { recursive: true });
      await fs.mkdir(path.dirname(generatedFile), { recursive: true });
      await fs.mkdir(leafSource, { recursive: true });
      await Promise.all([
        fs.writeFile(entryFile, '', 'utf8'),
        fs.writeFile(libraryEntryFile, "'use static'\n", 'utf8'),
        fs.writeFile(dynamicLibraryEntryFile, '', 'utf8'),
        fs.writeFile(libraryFile, "'use static'\n", 'utf8'),
        fs.writeFile(declarationFile, "'use static'\n", 'utf8'),
        fs.writeFile(dynamicFile, '', 'utf8'),
        fs.writeFile(generatedFile, "'use static'\n", 'utf8'),
        fs.writeFile(selectedLeafFile, '', 'utf8'),
        fs.writeFile(ownLeafFile, '', 'utf8'),
        fs.writeFile(path.join(leafSource, 'Unselected.ets'), "'use static'\n", 'utf8'),
        fs.writeFile(path.join(librarySource, 'Ignored.ETS'), '', 'utf8'),
        fs.writeFile(path.join(librarySource, 'Ignored.ts'), '', 'utf8'),
      ]);
      await fs.writeFile(
        path.join(entryRoot, 'interop.json5'),
        `{
                    // Dynamic paths belong to another consumer.
                    interopEntries: {
                        static: ['Index.ets', './Index.ets'],
                        dynamic: ['Missing.ets'],
                        dependency: {
                            package: ['library', 'dynamic-library'],
                            source: {
                                leaf: {
                                    static: ['src/Selected.ets'],
                                    dynamic: ['src/Missing.ets'],
                                },
                            },
                        },
                    },
                }`,
        'utf8',
      );
      await fs.writeFile(
        path.join(leafRoot, 'interop.json5'),
        `{ interopEntries: { static: ['src/Own.ets'] } }`,
        'utf8',
      );

      const buildConfig: BuildConfig = {
        plugins: [],
        buildMode: 'Debug',
        buildType: 'BUILD',
        projectRootPath: projectRoot,
        cachePath: 'cache',
        compileSdkVersion: 10,
        compatibleSdkVersion: 10,
        bundleName: 'com.example.app',
        moduleType: 'entry',
        moduleName: 'entry',
        packageName: 'entry',
        buildSdkPath: 'sdk/build',
        dependentModuleList: [
          {
            packageName: 'entry',
            moduleType: 'entry',
            modulePath: entryRoot,
            sourceRoots: ['.'],
            entryFile: 'Index.ets',
            dependencies: ['library', 'dynamic-library'],
            interopConfigPath: 'interop.json5',
          },
          {
            packageName: 'library',
            moduleType: 'har',
            modulePath: libraryRoot,
            sourceRoots: ['missing', 'src', '.'],
            entryFile: 'Index.ets',
            dependencies: ['leaf'],
          },
          {
            packageName: 'leaf',
            moduleType: 'har',
            modulePath: leafRoot,
            sourceRoots: ['src'],
            entryFile: 'Index.ets',
            interopConfigPath: 'interop.json5',
          },
          {
            packageName: 'dynamic-library',
            moduleType: 'har',
            modulePath: dynamicLibraryRoot,
            sourceRoots: ['.'],
            entryFile: 'Index.ets',
            dependencies: [],
          },
        ],
        hasMainModule: true,
        modulePath: entryRoot,
        externalApiPaths: [],
        byteCodeHar: false,
        interopApiPaths: [],
        declgenBridgeConfigPath: path.join(projectRoot, 'declgen-bridge.json'),
        interopConfigPath: path.join(projectRoot, 'unused-project-config.json5'),
      };
      const context: GlueGenContext = {
        buildConfig,
        logger: SILENT_LOGGER,
        runtime: { nativeExecutablePath: '' },
      };
      const expectedPath = path.join(projectRoot, 'cache', 'gluegen', 'fileInfo.txt');
      await fs.mkdir(path.dirname(expectedPath), { recursive: true });
      await fs.writeFile(expectedPath, 'stale.ets', 'utf8');

      const outputPath = await Pipeline.start<GlueGenContext>()
        .stage(createConfigurationStage())
        .stage(createPrepareStage())
        .run(context);

      expect(outputPath).toBe(expectedPath);
      const fileList = (await fs.readFile(outputPath, 'utf8')).split('\n').filter(Boolean);
      expect(fileList).toHaveLength(6);
      const files = new Set(fileList);
      expect(files).toEqual(
        new Set([entryFile, libraryEntryFile, libraryFile, declarationFile, selectedLeafFile, ownLeafFile]),
      );
      expect(files).not.toContain(dynamicLibraryEntryFile);
      expect(files).not.toContain(generatedFile);
    } finally {
      await fs.rm(projectRoot, { recursive: true, force: true });
    }
  });

  it('reports an unknown interop dependency with structured package context', async () => {
    const projectRoot = await fs.mkdtemp(path.join(tmpdir(), 'gluegen-interop-error-'));
    try {
      const entryRoot = path.join(projectRoot, 'entry');
      const interopConfigPath = path.join(entryRoot, 'interop.json5');
      await fs.mkdir(entryRoot, { recursive: true });
      await Promise.all([
        fs.writeFile(path.join(entryRoot, 'Index.ets'), '', 'utf8'),
        fs.writeFile(interopConfigPath, `{ interopEntries: { dependency: { package: ['stalib'] } } }`, 'utf8'),
      ]);
      const logger: jest.Mocked<ILogger> = {
        printInfo: jest.fn(),
        printWarn: jest.fn(),
        printDebug: jest.fn(),
        printError: jest.fn(),
        printErrorAndExit: jest.fn(),
      };
      const buildConfig: BuildConfig = {
        plugins: [],
        buildMode: 'Debug',
        buildType: 'BUILD',
        projectRootPath: projectRoot,
        cachePath: 'cache',
        compileSdkVersion: 10,
        compatibleSdkVersion: 10,
        bundleName: 'com.example.app',
        moduleType: 'entry',
        moduleName: 'entry',
        packageName: 'entry',
        buildSdkPath: 'sdk/build',
        dependentModuleList: [
          {
            packageName: 'entry',
            moduleType: 'entry',
            modulePath: entryRoot,
            sourceRoots: ['.'],
            entryFile: 'Index.ets',
            dependencies: [],
            interopConfigPath: 'interop.json5',
          },
        ],
        hasMainModule: true,
        modulePath: entryRoot,
        externalApiPaths: [],
        byteCodeHar: false,
        interopApiPaths: [],
        declgenBridgeConfigPath: path.join(projectRoot, 'declgen-bridge.json'),
        interopConfigPath: '',
      };
      const context: GlueGenContext = {
        buildConfig,
        logger,
        runtime: { nativeExecutablePath: '' },
      };

      const error = await Pipeline.start<GlueGenContext>()
        .stage(createConfigurationStage())
        .run(context)
        .then(
          (): undefined => undefined,
          (failure: unknown) => failure,
        );

      expect(error).toBeInstanceOf(GlueGenDiagnosticError);
      const diagnostic = (error as GlueGenDiagnosticError).logData;
      expect(diagnostic).toMatchObject({
        code: '11420002',
        description: 'Package "entry" does not have a dependency named "stalib".',
        cause: 'No package named "stalib" is reachable in the Main Module dependency graph.',
        position: interopConfigPath,
        solutions: ['Add "stalib" to the module dependencies, or remove the interop reference.'],
        moreInfo: {
          packageName: 'entry',
          dependencyName: 'stalib',
        },
      });
      expect(logger.printError).not.toHaveBeenCalled();
    } finally {
      await fs.rm(projectRoot, { recursive: true, force: true });
    }
  });
});
