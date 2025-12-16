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

import * as ts from 'typescript';
import type { CommandLineOptions } from '../CommandLineOptions';
import { createCompilerHost, readDeclareFiles } from './ResolveSdks';
import * as path from 'node:path';

function getTargetESVersionLib(optionsTarget: ts.ScriptTarget): string[] {
  switch (optionsTarget) {
    case ts.ScriptTarget.ES2017:
      return ['lib.es2017.d.ts'];
    case ts.ScriptTarget.ES2021:
      return ['lib.es2021.d.ts'];
    default:
      return ['lib.es2021.d.ts'];
  }
}

export function formTscOptions(
  cmdOptions: CommandLineOptions,
  overrideCompilerOptions: ts.CompilerOptions
): ts.CreateProgramOptions {
  let options: ts.CreateProgramOptions;
  if (cmdOptions.parsedConfigFile) {
    options = {
      rootNames: cmdOptions.inputFiles.concat(readDeclareFiles(cmdOptions.sdkDefaultApiPath ?? '')),
      options: cmdOptions.parsedConfigFile.options,
      projectReferences: cmdOptions.parsedConfigFile.projectReferences,
      configFileParsingDiagnostics: ts.getConfigFileParsingDiagnostics(cmdOptions.parsedConfigFile)
    };
    Object.assign(options.options, getDefaultCompilerOptions());
  } else {
    const rootNames = cmdOptions.inputFiles.concat(readDeclareFiles(cmdOptions.sdkDefaultApiPath ?? ''));
    const ESVersion = cmdOptions.followSdkSettings ? ts.ScriptTarget.ES2021 : ts.ScriptTarget.Latest;
    const ESVersionLib = cmdOptions.followSdkSettings ? getTargetESVersionLib(ESVersion) : undefined;
    options = {
      rootNames: rootNames,
      options: {
        target: ESVersion,
        module: ts.ModuleKind.CommonJS,
        allowJs: true,
        checkJs: !cmdOptions.followSdkSettings,
        lib: ESVersionLib
      }
    };
  }
  if (cmdOptions.sdkDefaultApiPath) {
    const etsLoaderPath = path.resolve(cmdOptions.sdkDefaultApiPath, './build-tools/ets-loader');
    Object.assign(options.options, {
      etsLoaderPath: etsLoaderPath
    });
  }
  options.options = Object.assign(options.options, overrideCompilerOptions);
  if (cmdOptions.sdkDefaultApiPath && cmdOptions.arktsWholeProjectPath && cmdOptions.sdkExternalApiPath) {
    options.host = createCompilerHost(
      cmdOptions.sdkDefaultApiPath,
      cmdOptions.sdkExternalApiPath,
      cmdOptions.arktsWholeProjectPath,
      options.options
    );
  }
  setNoTransformedKitOption(options.options);
  return options;
}

function setNoTransformedKitOption(options: ts.CompilerOptions): void {
  options.noTransformedKitInParser = true;
}

function getDefaultCompilerOptions(): ts.CompilerOptions {
  return {
    allowJs: true,
    checkJs: false,
    emitNodeModulesFiles: true,
    importsNotUsedAsValues: ts.ImportsNotUsedAsValues.Preserve,
    module: ts.ModuleKind.CommonJS,
    moduleResolution: ts.ModuleResolutionKind.NodeJs,
    noEmit: true,
    maxFlowDepth: 2000,
    types: [],
    incremental: true,
    needDoArkTsLinter: true
  };
}
