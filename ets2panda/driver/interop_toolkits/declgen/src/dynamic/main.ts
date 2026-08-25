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

import * as ts from 'typescript';
import { Logger } from './logger/logger';
import { ConsoleLogger } from './logger/consoleLogger';

import { CLI } from './cli/cli';
import { DeclgenCLI, DeclgenCLIOptions } from './cli/declgenCLI';
import { getSourceFilesFromDir, parseConfigFile, defaultCompilerOptions } from './compiler/compiler';

import { Declgen, DeclgenOptions } from './declgen';

function main(): void {
  Logger.init(new ConsoleLogger());

  const parsedCliOptions = CLI.parseCLI(new DeclgenCLI());

  const { rootNames, options } = parseDeclgenOptions(parsedCliOptions);

  const declgenOptions: DeclgenOptions = {
    rootDir: parsedCliOptions.rootDir,
    inputFiles: rootNames,
    outDir: parsedCliOptions.outDir,
    features: {
      enableInteropTypesFix: parsedCliOptions.enableInteropTypesFix ?? false,
      removeReservedKeywordIdentifier: parsedCliOptions.removeReservedKeywordIdentifier ?? false,
    },
    incremental: parsedCliOptions.incremental,
    cacheDir: parsedCliOptions.cacheDir,
    noCache: parsedCliOptions.noCache,
    verifyOutputs: parsedCliOptions.verifyOutputs,
  };

  const declgen = new Declgen(declgenOptions, options);

  declgen.run().emit();
}

function collectInputFiles(opts: DeclgenCLIOptions): string[] {
  const inputFiles = [] as string[];

  if (opts.inputFiles) {
    inputFiles.push(...opts.inputFiles);
  }

  if (opts.inputDirs) {
    for (const dir of opts.inputDirs) {
      try {
        inputFiles.push(...getSourceFilesFromDir(dir));
      } catch (error) {
        Logger.error('Failed to read folder: ' + error);
        process.exit(-1);
      }
    }
  }

  return inputFiles;
}

function parseDeclgenOptions(opts: DeclgenCLIOptions): ts.CreateProgramOptions {
  const parsedConfigFile = opts.tsconfig ? parseConfigFile(opts.tsconfig) : undefined;

  if (parsedConfigFile) {
    return {
      rootNames: parsedConfigFile.fileNames,
      options: parsedConfigFile.options,
      projectReferences: parsedConfigFile.projectReferences,
      configFileParsingDiagnostics: ts.getConfigFileParsingDiagnostics(parsedConfigFile),
    };
  }
  return {
    rootNames: collectInputFiles(opts),
    options: defaultCompilerOptions(),
  };
}

main();
