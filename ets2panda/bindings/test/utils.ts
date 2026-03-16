/*
 * Copyright (c) 2025-2026 Huawei Device Co., Ltd.
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

import fs from 'fs';
import JSON5 from 'json5';
import { Lsp, ModuleDescriptor, PathConfig } from '../src';
import path from 'path';

export const DEFAULT_PATH_CONFIG: PathConfig = {
  buildSdkPath: path.resolve('test', 'ets', 'static'),
  projectPath: path.resolve('test', 'testcases'),
  declgenOutDir: ''
};

const DEFAULT_PLUGIN_LIST: string[] = process.env.SKIP_UI_PLUGINS ? [] : ['ui-syntax-plugins', 'ui-plugins', 'memo-plugins'];
const STDLIB_PATH_KEYS = ['std', 'escompat', 'arkruntime'];

interface ParsedArkTSConfigObject {
  compilerOptions?: {
    baseUrl?: string;
    paths?: Record<string, unknown>;
  };
}

function normalizePathEntries(entries: string[], baseUrl?: string): string[] {
  return entries.map((entry) => {
    if (path.isAbsolute(entry)) {
      return path.resolve(entry);
    }
    if (baseUrl) {
      return path.resolve(baseUrl, entry);
    }
    return path.resolve(entry);
  });
}

function getGlobalArkTSPaths(): Record<string, string[]> {
  const globalArkTSConfigPath = process.env.ARKTSCONFIG;
  if (!globalArkTSConfigPath || !fs.existsSync(globalArkTSConfigPath)) {
    return {};
  }

  try {
    const rawContent = fs.readFileSync(globalArkTSConfigPath, 'utf-8');
    const parsed = JSON5.parse(rawContent) as ParsedArkTSConfigObject;
    const baseUrl = parsed.compilerOptions?.baseUrl;
    const rawPaths = parsed.compilerOptions?.paths ?? {};
    const normalizedPaths: Record<string, string[]> = {};

    Object.entries(rawPaths).forEach(([key, value]) => {
      if (!Array.isArray(value)) {
        return;
      }
      const stringEntries = value.filter((item): item is string => typeof item === 'string');
      if (stringEntries.length === 0) {
        return;
      }
      normalizedPaths[key] = normalizePathEntries(stringEntries, baseUrl);
    });

    return normalizedPaths;
  } catch {
    return {};
  }
}

function hasUsablePathEntries(entries?: string[]): boolean {
  return Array.isArray(entries) && entries.length > 0 && entries.some((entry) => fs.existsSync(entry));
}

function patchGeneratedArkTsConfigsForTest(lsp: Lsp): void {
  const globalPaths = getGlobalArkTSPaths();
  if (Object.keys(globalPaths).length === 0) {
    return;
  }

  const moduleInfos = (lsp as any).moduleInfos as Record<string, { arktsConfigFile?: string }> | undefined;
  if (!moduleInfos) {
    return;
  }

  const arktsConfigFiles = Array.from(
    new Set(
      Object.values(moduleInfos)
        .map((moduleInfo) => moduleInfo?.arktsConfigFile)
        .filter((filePath): filePath is string => typeof filePath === 'string' && fs.existsSync(filePath))
    )
  );

  arktsConfigFiles.forEach((arktsConfigFile) => {
    try {
      const rawContent = fs.readFileSync(arktsConfigFile, 'utf-8');
      const parsed = JSON5.parse(rawContent) as ParsedArkTSConfigObject;
      const compilerOptions = parsed.compilerOptions ?? {};
      const pathSection = (compilerOptions.paths ?? {}) as Record<string, string[]>;

      Object.entries(globalPaths).forEach(([key, value]) => {
        if (!hasUsablePathEntries(pathSection[key]) && hasUsablePathEntries(value)) {
          pathSection[key] = value;
        }
      });

      STDLIB_PATH_KEYS.forEach((key) => {
        if (!hasUsablePathEntries(pathSection[key])) {
          delete pathSection[key];
        }
      });

      compilerOptions.paths = pathSection;
      parsed.compilerOptions = compilerOptions;
      fs.writeFileSync(arktsConfigFile, JSON.stringify(parsed, null, 2), 'utf-8');
    } catch {
      // Keep test helper best-effort to avoid masking original test failures.
    }
  });
}

function initLspEnv(pathConfig: PathConfig): void {
  const bindingsPath =
    process.env.BINDINGS_PATH || path.join(pathConfig.buildSdkPath, 'build-tools', 'bindings');

  const pandaLibPath =
    process.env.PANDA_LIB_PATH || path.join(pathConfig.buildSdkPath, 'build-tools', 'bindings');

  const pandaBinPath =
    process.env.PANDA_BIN_PATH || path.join(pathConfig.buildSdkPath, 'build-tools', 'ets2panda', 'bin');

  process.env.BINDINGS_PATH = bindingsPath;
  process.env.PANDA_LIB_PATH = pandaLibPath;
  process.env.PANDA_BIN_PATH = pandaBinPath;
}

export function getLsp(moduleName: string, plugins: string[] = DEFAULT_PLUGIN_LIST): Lsp {
  initLspEnv(DEFAULT_PATH_CONFIG);
  return new Lsp(
    DEFAULT_PATH_CONFIG,
    undefined,
    [{ name: moduleName, moduleType: 'har', srcPath: path.resolve('test/testcases/', moduleName) }],
    plugins
  );
}

export function getMultiModuleLsp(
  projectName: string,
  modules: ModuleDescriptor[],
  plugins: string[] = DEFAULT_PLUGIN_LIST,
  pathConfigOverrides: Partial<PathConfig> = {}
): Lsp {
  const projectPath = path.resolve('test/testcases/', projectName);
  const pathConfig: PathConfig = {
    ...DEFAULT_PATH_CONFIG,
    projectPath,
    ...pathConfigOverrides
  };
  initLspEnv(pathConfig);

  const resolvedModules = modules.map((module) => ({
    ...module,
    srcPath: path.resolve(projectPath, module.srcPath)
  }));

  const lsp = new Lsp(pathConfig, undefined, resolvedModules, plugins);
  patchGeneratedArkTsConfigsForTest(lsp);
  return lsp;
}

export function getRealPath(moduleName: string, fileName: string): string {
  return path.resolve('test/testcases/', moduleName, fileName);
}
